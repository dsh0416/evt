#ifndef URING_H
#define URING_H
#include "evt.h"
#if HAVE_LIBURING_H
void uring_payload_free(void* data) {
    // TODO: free the uring_data structs if the payload is freed before all IO responds
    io_uring_queue_exit((struct io_uring*) data);
    xfree(data);
}

size_t uring_payload_size(const void* data) {
    return sizeof(struct io_uring);
}

VALUE method_scheduler_uring_init(VALUE self) {
    int ret;
    struct io_uring* ring;
    ring = xmalloc(sizeof(struct io_uring));
    ret = io_uring_queue_init(URING_ENTRIES, ring, 0);
    if (ret < 0) {
        rb_raise(rb_eIOError, "unable to initalize io_uring");
    }
    rb_iv_set(self, "@ring", TypedData_Wrap_Struct(Payload, &type_uring_payload, ring));
    return Qnil;
}

VALUE method_scheduler_uring_register(VALUE self, VALUE io, VALUE interest) {
    VALUE ring_obj;
    struct io_uring* ring;
    struct io_uring_sqe *sqe;
    struct uring_data *data;
    short poll_mask = 0;
    ID id_fileno = rb_intern("fileno");

    ring_obj = rb_iv_get(self, "@ring");
    TypedData_Get_Struct(ring_obj, struct io_uring, &type_uring_payload, ring);
    sqe = io_uring_get_sqe(ring);
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));

    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));

    if (ruby_interest & readable) {
        poll_mask |= POLL_IN;
    }

    if (ruby_interest & writable) {
        poll_mask |= POLL_OUT;
    }

    data = (struct uring_data*) xmalloc(sizeof(struct uring_data));
    data->is_poll = true;
    data->io = io;
    data->poll_mask = poll_mask;
    
    io_uring_prep_poll_add(sqe, fd, poll_mask);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);
    return Qnil;
}

VALUE method_scheduler_uring_wait(VALUE self) {
    struct io_uring* ring;
    struct io_uring_cqe *cqes[URING_MAX_EVENTS];
    struct uring_data *data;
    VALUE next_timeout, obj_io, readables, writables, iovs, result;
    unsigned ret, i;
    double time = 0.0;
    short poll_events;

    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");
    ID id_sleep = rb_intern("sleep");

    next_timeout = rb_funcall(self, id_next_timeout, 0);
    readables = rb_ary_new();
    writables = rb_ary_new();
    iovs = rb_ary_new();

    TypedData_Get_Struct(rb_iv_get(self, "@ring"), struct io_uring, &type_uring_payload, ring);

    struct __kernel_timespec ts;
    ts.tv_sec = NUM2INT(next_timeout);
    ts.tv_nsec = 0;

    io_uring_wait_cqe_timeout(ring, cqes, &ts);
    ret = io_uring_peek_batch_cqe(ring, cqes, URING_MAX_EVENTS);

    for (i = 0; i < ret; i++) {
        data = (struct uring_data*) io_uring_cqe_get_data(cqes[i]);
        poll_events = data->poll_mask;
        obj_io = data->io;
        if (data->is_poll) {
            if (poll_events & POLL_IN) {
                rb_funcall(readables, id_push, 1, obj_io);
            }
            
            if (poll_events & POLL_OUT) {
                rb_funcall(writables, id_push, 1, obj_io);
            }
        } else {
            rb_funcall(iovs, id_push, 1, obj_io);
        }
        io_uring_cqe_seen(ring, cqes[i]);
    }

    result = rb_ary_new2(3);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);
    rb_ary_store(result, 2, iovs);

    return result;
}

VALUE method_scheduler_uring_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    struct io_uring* ring;
    struct uring_data *data;
    char* read_buffer;
    ID id_fileno = rb_intern("fileno");
    // @iov[io] = Fiber.current
    VALUE iovs = rb_iv_get(self, "@iovs");
    rb_hash_aset(iovs, io, rb_funcall(Fiber, rb_intern("current"), 0));
    // register
    VALUE ring_obj = rb_iv_get(self, "@ring");
    TypedData_Get_Struct(ring_obj, struct io_uring, &type_uring_payload, ring);
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));

    read_buffer = (char*) xmalloc(NUM2SIZET(length));

    data = (struct uring_data*) xmalloc(sizeof(struct uring_data));
    data->is_poll = false;
    data->io = io;
    data->poll_mask = 0;
    
    io_uring_prep_read(sqe, fd, read_buffer, 1, NUM2SIZET(length), NUM2SIZET(offset));
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);

    rb_funcall(Fiber, rb_intern("yield"), 0); // Fiber.yield

    VALUE result = rb_str_new(read_buffer, strlen(read_buffer));
    if (buffer != Qnil) {
        rb_str_append(buffer, result);
    }
    return SIZET2NUM(strlen(read_buffer));
}

VALUE method_scheduler_uring_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    struct io_uring* ring;
    struct uring_data *data;
    char* write_buffer;
    ID id_fileno = rb_intern("fileno");
    // @iov[io] = Fiber.current
    VALUE iovs = rb_iv_get(self, "@iovs");
    rb_hash_aset(iovs, io, rb_funcall(Fiber, rb_intern("current"), 0));
    // register
    VALUE ring_obj = rb_iv_get(self, "@ring");
    TypedData_Get_Struct(ring_obj, struct io_uring, &type_uring_payload, ring);
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));

    write_buffer = StringValueCStr(buffer);

    data = (struct uring_data*) xmalloc(sizeof(struct uring_data));
    data->is_poll = false;
    data->io = io;
    data->poll_mask = 0;
    
    io_uring_prep_write(sqe, fd, write_buffer, NUM2SIZET(length), NUM2SIZET(offset));
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);
    rb_funcall(Fiber, rb_intern("yield"), 0); // Fiber.yield
    return length;
}

VALUE method_scheduler_uring_backend(VALUE klass) {
    return rb_str_new_cstr("liburing");
}

#endif
#endif

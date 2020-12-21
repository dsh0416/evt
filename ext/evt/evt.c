#include "evt.h"

void Init_evt_ext()
{
    Evt = rb_define_module("Evt");
    Scheduler = rb_define_class_under(Evt, "Scheduler", rb_cObject);
    Payload = rb_define_class_under(Scheduler, "Payload", rb_cObject);
    Fiber = rb_define_class("Fiber", rb_cObject);
    rb_define_singleton_method(Scheduler, "backend", method_scheduler_backend, 0);
    rb_define_method(Scheduler, "init_selector", method_scheduler_init, 0);
    rb_define_method(Scheduler, "register", method_scheduler_register, 2);
    rb_define_method(Scheduler, "deregister", method_scheduler_deregister, 1);
    rb_define_method(Scheduler, "wait", method_scheduler_wait, 0);

#if HAVELIBURING_H || HAVE_WINDOWS_H
    rb_define_method(Scheduler, "io_read", method_scheduler_io_read, 4);
    rb_define_method(Scheduler, "io_write", method_scheduler_io_read, 4);
#endif
}

#if HAVE_LIBURING_H
void uring_payload_free(void* data) {
    io_uring_queue_exit((struct io_uring*) data);
    xfree(data);
}

size_t uring_payload_size(const void* data) {
    return sizeof(struct io_uring);
}

VALUE method_scheduler_init(VALUE self) {
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

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
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
    data->io = (void*)io;
    data->poll_mask = poll_mask;
    
    io_uring_prep_poll_add(sqe, fd, poll_mask);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    // io_uring runs under oneshot mode. No need to deregister.
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
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
    ret = io_uring_peek_batch_cqe(ring, cqes, URING_MAX_EVENTS);

    for (i = 0; i < ret; i++) {
        data = (struct uring_data*) io_uring_cqe_get_data(cqes[i]);
        poll_events = data->poll_mask;
        if (!data->is_poll) {
            obj_io = (VALUE) data->io;
            rb_funcall(iovs, id_push, 1, obj_io);
        } else if (poll_events & POLL_IN) {
            obj_io = (VALUE) data->io;
            rb_funcall(readables, id_push, 1, obj_io);
        } else if (poll_events & POLL_OUT) {
            obj_io = (VALUE) data->io;
            rb_funcall(writables, id_push, 1, obj_io);
        }
    }

    if (ret == 0) {
       if (next_timeout != Qnil && NUM2INT(next_timeout) != -1) {
            // sleep
            time = next_timeout / 1000;
            rb_funcall(rb_mKernel, id_sleep, 1, RFLOAT_VALUE(time));
       } else {
            rb_funcall(rb_mKernel, id_sleep, 1, RFLOAT_VALUE(0.001)); // To avoid infinite loop
       }
    }

    result = rb_ary_new2(3);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);
    rb_ary_store(result, 2, iovs);

    return result;
}

VALUE method_scheduler_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
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
    struct iovec iov = {
        .iov_base = read_buffer,
        .iov_len = NUM2SIZET(length),
    };

    data = (struct uring_data*) xmalloc(sizeof(struct uring_data));
    data->is_poll = false;
    data->io = (void*)io;
    data->poll_mask = 0;
    
    io_uring_prep_readv(sqe, fd, &iov, 1, NUM2SIZET(offset));
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);

    VALUE result = rb_str_new(read_buffer, strlen(read_buffer));
    xfree(read_buffer);
    if (buffer != Qnil) {
        rb_str_append(buffer, result);
    }

    rb_funcall(Fiber, rb_intern("yield"), 0); // Fiber.yield
    return result;
}

VALUE method_scheduler_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
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
    struct iovec iov = {
        .iov_base = write_buffer,
        .iov_len = NUM2SIZET(length),
    };

    data = (struct uring_data*) xmalloc(sizeof(struct uring_data));
    data->is_poll = false;
    data->io = (void*)io;
    data->poll_mask = 0;
    
    io_uring_prep_writev(sqe, fd, &iov, 1, NUM2SIZET(offset));
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);
    rb_funcall(Fiber, rb_intern("yield"), 0); // Fiber.yield
    return length;
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("liburing");
}

#elif HAVE_SYS_EPOLL_H // TODO: Do more checks for using epoll
VALUE method_scheduler_init(VALUE self) {
    rb_iv_set(self, "@epfd", INT2NUM(epoll_create(1))); // Size of epoll is ignored after Linux 2.6.8.
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    struct epoll_event event;
    ID id_fileno = rb_intern("fileno");
    int epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));
    
    if (ruby_interest & readable) {
        event.events |= EPOLLIN;
    }

    if (ruby_interest & writable) {
        event.events |= EPOLLOUT;
    }

    event.data.ptr = (void*) io;

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    ID id_fileno = rb_intern("fileno");
    int epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // Require Linux 2.6.9 for NULL event.
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    int n, epfd, i, event_flag, timeout;
    VALUE next_timeout, obj_io, readables, writables, result;
    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");
    
    epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    next_timeout = rb_funcall(self, id_next_timeout, 0);
    readables = rb_ary_new();
    writables = rb_ary_new();

    if (next_timeout == Qnil) {
        timeout = -1;
    } else {
        timeout = NUM2INT(next_timeout);
    }

    struct epoll_event* events = (struct epoll_event*) xmalloc(sizeof(struct epoll_event) * EPOLL_MAX_EVENTS);
    
    n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, timeout);
    if (n < 0) {
        rb_raise(rb_eIOError, "unable to call epoll_wait");
    }

    for (i = 0; i < n; i++) {
        event_flag = events[i].events;
        if (event_flag & EPOLLIN) {
            obj_io = (VALUE) events[i].data.ptr;
            rb_funcall(readables, id_push, 1, obj_io);
        } else if (event_flag & EPOLLOUT) {
            obj_io = (VALUE) events[i].data.ptr;
            rb_funcall(writables, id_push, 1, obj_io);
        }
    }

    result = rb_ary_new2(2);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);

    xfree(events);
    return result;
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("epoll");
}
#elif HAVE_SYS_EVENT_H
VALUE method_scheduler_init(VALUE self) {
    rb_iv_set(self, "@kq", INT2NUM(kqueue()));
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    struct kevent event;
    u_short event_flags = 0;
    ID id_fileno = rb_intern("fileno");
    int kq = NUM2INT(rb_iv_get(self, "@kq"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));
    
    if (ruby_interest & readable) {
        event_flags |= EVFILT_READ;
    }

    if (ruby_interest & writable) {
        event_flags |= EVFILT_WRITE;
    }

    EV_SET(&event, fd, event_flags, EV_ADD|EV_ENABLE, 0, 0, (void*) io);
    kevent(kq, &event, 1, NULL, 0, NULL); // TODO: Check the return value
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    struct kevent event;
    ID id_fileno = rb_intern("fileno");
    int kq = NUM2INT(rb_iv_get(self, "@kq"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    EV_SET(&event, fd, 0, EV_DELETE, 0, 0, NULL);
    kevent(kq, &event, 1, NULL, 0, NULL); // TODO: Check the return value
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    int n, kq, i;
    u_short event_flags = 0;

    struct kevent* events; // Event Triggered
    struct timespec timeout;
    VALUE next_timeout, obj_io, readables, writables, result;
    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");

    kq = NUM2INT(rb_iv_get(self, "@kq"));
    next_timeout = rb_funcall(self, id_next_timeout, 0);
    readables = rb_ary_new();
    writables = rb_ary_new();

   events = (struct kevent*) xmalloc(sizeof(struct kevent) * KQUEUE_MAX_EVENTS);

    if (next_timeout == Qnil || NUM2INT(next_timeout) == -1) {
        n = kevent(kq, NULL, 0, events, KQUEUE_MAX_EVENTS, NULL);
    } else {
        timeout.tv_sec = next_timeout / 1000;
        timeout.tv_nsec = next_timeout % 1000 * 1000 * 1000;
        n = kevent(kq, NULL, 0, events, KQUEUE_MAX_EVENTS, &timeout);
    }

    // TODO: Check if n >= 0
    for (i = 0; i < n; i++) {
        event_flags = events[i].filter;
        if (event_flags & EVFILT_READ) {
            obj_io = (VALUE) events[i].udata;
            rb_funcall(readables, id_push, 1, obj_io);
        } else if (event_flags & EVFILT_WRITE) {
            obj_io = (VALUE) events[i].udata;
            rb_funcall(writables, id_push, 1, obj_io);
        }
    }

    result = rb_ary_new2(2);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);

    xfree(events);
    return result;
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("kqueue");
}
#elif HAVE_WINDOWS_H
void iocp_payload_free(void* data) {
    CloseHandle((HANDLE) data);
}

size_t iocp_payload_size(const void* data) {
    return sizeof(HANDLE);
}

VALUE method_scheduler_init(VALUE self) {
    int ret;
    HANDLE iocp;
    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    rb_iv_set(self, "@iocp", TypedData_Wrap_Struct(Payload, &type_iocp_payload, iocp));
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    HANDLE iocp;
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);
    int fd = NUM2INT(rb_funcallv(io, rb_intern("fileno"), 0, 0));
    HANDLE io_handler = (HANDLE)rb_w32_get_osfhandle(fd);
    CreateIoCompletionPort(io_handler, iocp, (ULONG_PTR) io, 0);
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    // iocp runs under oneshot mode. No need to deregister.
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    HANDLE iocp;
    DWORD timeout;
    ID id_next_timeout = rb_intern("next_timeout");
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    VALUE next_timeout = rb_funcall(self, id_next_timeout, 0);

    LPOVERLAPPED_ENTRY lpCompletionPortEntries;
    PULONG ulNumEntriesRemoved;
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);

    if (next_timeout == Qnil) {
        timeout = -1;
    } else {
        timeout = NUM2INT(next_timeout) * 1000; // seconds to milliseconds
    }

    BOOL result = GetQueuedCompletionStatusEx(
        iocp, lpCompletionPortEntries, IOCP_MAX_EVENTS, ulNumEntriesRemoved, timeout, FALSE);

    // How to check if a IO is written or read???
    return Qnil;
}

VALUE method_scheduler_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    return Qnil;
}

VALUE method_scheduler_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    return Qnil;
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("iocp");
}
#else
// Fallback to IO.select
VALUE method_scheduler_init(VALUE self) {
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    // return IO.select(@readable.keys, @writable.keys, [], next_timeout)
    VALUE readable, writable, readable_keys, writable_keys, next_timeout;
    ID id_select = rb_intern("select");
    ID id_next_timeout = rb_intern("next_timeout");

    readable = rb_iv_get(self, "@readable");
    writable = rb_iv_get(self, "@writable");

    readable_keys = rb_funcall(readable, rb_intern("keys"), 0);
    writable_keys = rb_funcall(writable, rb_intern("keys"), 0);
    next_timeout = rb_funcall(self, id_next_timeout, 0);

    return rb_funcall(rb_cIO, id_select, 4, readable_keys, writable_keys, rb_ary_new(), next_timeout);
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("ruby");
}
#endif

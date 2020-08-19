#include "evt.h"

void Init_evt_ext()
{
    Evt = rb_define_module("Evt");
    Scheduler = rb_define_class_under(Evt, "Scheduler", rb_cObject);
    Payload = rb_define_class_under(Scheduler, "Payload", rb_cObject);
    rb_define_singleton_method(Scheduler, "backend", method_scheduler_backend, 0);
    rb_define_method(Scheduler, "init_selector", method_scheduler_init, 0);
    rb_define_method(Scheduler, "register", method_scheduler_register, 2);
    rb_define_method(Scheduler, "deregister", method_scheduler_deregister, 1);
    rb_define_method(Scheduler, "wait", method_scheduler_wait, 0);
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
    // printf("Address of ring is %p\n", (void *)ring);
    ret = io_uring_queue_init(URING_ENTRIES, ring, 0);
    if (ret < 0) {
        // TODO: check uring status
    }
    // printf("Address of ring is %p\n", (void *)ring);
    rb_iv_set(self, "@ring", TypedData_Wrap_Struct(Payload, &type_uring_payload, ring));
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    VALUE ring_obj;
    struct io_uring* ring;
    struct io_uring_sqe *sqe;
    struct uring_payload *payload;
    short poll_mask = 0;
    ID id_fileno = rb_intern("fileno");

    ring_obj = rb_iv_get(self, "@ring");
    TypedData_Get_Struct(ring_obj, struct io_uring, &type_uring_payload, ring);
    sqe = io_uring_get_sqe(ring);
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));

    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_WRITABLE")));
    
    if (ruby_interest & readable) {
        poll_mask |= POLL_IN;
    } else if (ruby_interest & writable) {
        poll_mask |= POLL_OUT;
    }

    payload = (struct uring_payload*) xmalloc(sizeof(struct uring_payload));
    payload->io = (void*)io;
    payload->poll_mask = poll_mask;
    
    io_uring_prep_poll_add(sqe, fd, poll_mask);
    io_uring_sqe_set_data(sqe, payload);
    io_uring_submit(ring);
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    // io_uring runs under onshot mode. No need to deregister.
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    struct io_uring* ring;
    struct io_uring_cqe *cqes[URING_MAX_EVENTS];
    struct uring_payload *payload;
    VALUE next_timeout, obj_io, readables, writables, result;
    unsigned ret, i;
    double time = 0.0;
    short poll_events;

    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");
    ID id_sleep = rb_intern("sleep");

    next_timeout = rb_funcall(self, id_next_timeout, 0);
    readables = rb_ary_new();
    writables = rb_ary_new();

    TypedData_Get_Struct(rb_iv_get(self, "@ring"), struct io_uring, &type_uring_payload, ring);
    ret = io_uring_peek_batch_cqe(ring, cqes, URING_MAX_EVENTS);

    for (i = 0; i < ret; i++) {
        payload = (struct uring_payload*) io_uring_cqe_get_data(cqes[i]);
        poll_events = payload->poll_mask;
        if (poll_events & POLL_IN) {
            obj_io = (VALUE) payload->io;
            rb_funcall(readables, id_push, 1, obj_io);
        } else if (poll_events & POLL_OUT) {
            obj_io = (VALUE) payload->io;
            rb_funcall(writables, id_push, 1, obj_io);
        }
    }

    if (ret == 0) {
       if (next_timeout != Qnil && NUM2INT(next_timeout) != -1) {
            // sleep
            time = next_timeout / 1000;
            rb_funcall(rb_mKernel, id_sleep, 1, RFLOAT_VALUE(time));
       }
    }

    result = rb_ary_new2(2);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);

    return result;
}

VALUE method_scheduler_backend() {
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
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_WRITABLE")));
    
    if (ruby_interest & readable) {
        event.events |= EPOLLIN;
    } else if (ruby_interest & writable) {
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
    // TODO: Check if n >= 0

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

VALUE method_scheduler_backend() {
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
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WAIT_WRITABLE")));
    
    if (ruby_interest & readable) {
        event_flags |= EVFILT_READ;
    } else if (ruby_interest & writable) {
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

VALUE method_scheduler_backend() {
    return rb_str_new_cstr("kqueue");
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
    ID id_keys = rb_intern("keys");
    ID id_next_timeout = rb_intern("next_timeout");

    readable = rb_iv_get(self, "@readable");
    writable = rb_iv_get(self, "@writable");

    readable_keys = rb_funcall(readable, id_keys, 0);
    writable_keys = rb_funcall(writable, id_keys, 0);
    next_timeout = rb_funcall(self, id_next_timeout, 0);

    return rb_funcall(rb_cIO, id_select, 4, readable_keys, writable_keys, rb_ary_new(), next_timeout);
}

VALUE method_scheduler_backend() {
    return rb_str_new_cstr("ruby");
}
#endif

#include "evt.h"

void Init_evt_ext()
{
    Evt = rb_define_module("Evt");
    Scheduler = rb_define_class_under(Evt, "Scheduler", rb_cObject);
    rb_define_singleton_method(Scheduler, "backend", method_scheduler_backend, 0);
    rb_define_method(Scheduler, "init_selector", method_scheduler_init, 0);
    rb_define_method(Scheduler, "register", method_scheduler_register, 2);
    rb_define_method(Scheduler, "deregister", method_scheduler_deregister, 1);
    rb_define_method(Scheduler, "wait", method_scheduler_wait, 0);
}


#if defined(__linux__) // TODO: Do more checks for using epoll
#include <sys/epoll.h>
#define EPOLL_MAX_EVENTS 64

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
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/event.h>
#define KQUEUE_MAX_EVENTS 64

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
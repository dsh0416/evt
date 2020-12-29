#ifndef EPOLL_H
#define EPOLL_H
#include "evt.h"

#if HAVE_SYS_EPOLL_H

VALUE method_scheduler_epoll_init(VALUE self) {
    rb_iv_set(self, "@epfd", INT2NUM(epoll_create(1))); // Size of epoll is ignored after Linux 2.6.8.
    return Qnil;
}

VALUE method_scheduler_epoll_register(VALUE self, VALUE io, VALUE interest) {
    struct epoll_event event;
    ID id_fileno = rb_intern("fileno");
    int epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int priority = NUM2INT(rb_const_get(rb_cIO, rb_intern("PRIORITY")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));

    if (ruby_interest & readable) {
        event.events |= EPOLLIN;
    }

    if (ruby_interest & priority) {
        event.events |= EPOLLPRI;
    }

    if (ruby_interest & writable) {
        event.events |= EPOLLOUT;
    }

    event.events |= EPOLLRDHUP;

    event.data.ptr = (void*) io;

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    return Qnil;
}

VALUE method_scheduler_epoll_wait(VALUE self) {
    int n, epfd, i, event_flag, timeout;
    VALUE next_timeout, obj_io, iovs, result;
    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");

    epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    next_timeout = rb_funcall(self, id_next_timeout, 0);
    iovs = rb_ary_new();

    if (next_timeout == Qnil) {
        timeout = -1;
    } else {
        timeout = NUM2INT(next_timeout);
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];

    n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, timeout);
    if (n < 0) {
        rb_raise(rb_eIOError, "unable to call epoll_wait");
    }

    for (i = 0; i < n; i++) {
        event_flag = events[i].events;
        obj_io = (VALUE) events[i].data.ptr;
        VALUE e = rb_ary_new2(2);
        rb_ary_store(e, 0, obj_io);
        rb_ary_store(e, 1, INT2NUM(event_flag));
        rb_funcall(iovs, id_push, 1, e);
    }

    result = rb_ary_new2(3);
    rb_ary_store(result, 0, rb_ary_new());
    rb_ary_store(result, 1, rb_ary_new());
    rb_ary_store(result, 2, iovs);
    return result;
}

VALUE method_scheduler_epoll_deregister(VALUE self, VALUE io) {
    ID id_fileno = rb_intern("fileno");
    int epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    int fd = NUM2INT(rb_funcall(io, id_fileno, 0));
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // Require Linux 2.6.9 for NULL event.
    return Qnil;
}

VALUE method_scheduler_epoll_backend(VALUE klass) {
    return rb_str_new_cstr("epoll");
}
#endif
#endif
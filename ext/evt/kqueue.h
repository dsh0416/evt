#ifndef KQUEUE_H
#define KQUEUE_H
#include "evt.h"

#if HAVE_SYS_EVENT_H

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
#endif
#endif

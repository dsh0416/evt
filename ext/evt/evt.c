#pragma once
#ifndef EVT_C
#define EVT_C

#include <ruby.h>

VALUE Scheduler = Qnil;

void Init_evt_ext();
VALUE method_scheduler_init(VALUE self);
VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest);
VALUE method_scheduler_deregister(VALUE self, VALUE io);
VALUE method_scheduler_wait(VALUE self);

void Init_evt_ext()
{
  Scheduler = rb_define_module("Scheduler");
  rb_define_method(Scheduler, "init", method_scheduler_init, 0);
  rb_define_method(Scheduler, "register", method_scheduler_register, 2);
  rb_define_method(Scheduler, "deregister", method_scheduler_deregister, 1);
  rb_define_method(Scheduler, "wait", method_scheduler_wait, 0);
}

// #if defined(_WIN32)
//     // Include IOCP
// #elif defined(__linux__)
//     // Use epoll
// #elif defined(__FreeBSD__)
//     // Use kqueue
// #else
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

    readable = rb_iv_get(self, rb_str_new2("@readble"));
    writable = rb_iv_get(self, rb_str_new2("@writable"));

    readable_keys = rb_funcall(readable, id_keys, 0);
    writable_keys = rb_funcall(writable, id_keys, 0);
    next_timeout = rb_funcall(writable, id_next_timeout, 0);

    return rb_funcall(rb_cIO, id_select, 4, readable_keys, writable_keys, rb_ary_new(), next_timeout);
}
#endif
// #endif

#ifndef SELECT_H
#define SELECT_H
#include "evt.h"

VALUE method_scheduler_select_wait(VALUE self) {
    // return IO.select(@readable.keys, @writable.keys, [], next_timeout)
    VALUE readable, writable, readable_keys, writable_keys, next_timeout;
    ID id_select = rb_intern("select");
    ID id_next_timeout = rb_intern("next_timeout");
    ID id_div = rb_intern("div");

    readable = rb_iv_get(self, "@readable");
    writable = rb_iv_get(self, "@writable");

    readable_keys = rb_funcall(readable, rb_intern("keys"), 0);
    writable_keys = rb_funcall(writable, rb_intern("keys"), 0);
    next_timeout = rb_funcall(self, id_next_timeout, 0);
    VALUE secs = rb_funcall(next_timeout, id_div, DBL2NUM(1000.0));

    return rb_funcall(rb_cIO, id_select, 4, readable_keys, writable_keys, rb_ary_new(), secs);
}

VALUE method_scheduler_select_backend(VALUE klass) {
    return rb_str_new_cstr("ruby");
}
#endif
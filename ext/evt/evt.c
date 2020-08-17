#include <ruby.h>

VALUE Scheduler = Qnil;

void Init_evt_ext();
VALUE method_scheduler_init(VALUE self);
VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest);
VALUE method_scheduler_deregister(VALUE self, VALUE io);
VALUE method_scheduler_wait(VALUE self);

void Init_evt_ext()
{
  Scheduler = rb_define_class("Scheduler", rb_cObject);
  rb_define_method(Scheduler, "init_selector", method_scheduler_init, 0);
  rb_define_method(Scheduler, "register", method_scheduler_register, 2);
  rb_define_method(Scheduler, "deregister", method_scheduler_deregister, 1);
  rb_define_method(Scheduler, "wait", method_scheduler_wait, 0);
}


#if defined(__linux__) // TODO: Do more checks for using epoll
#include <sys/epoll.h>
#define EPOLL_MAX_EVENTS 65535

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
    int n, epfd, i, event_flag;
    VALUE next_timeout, obj_io, readables, writables, result;
    ID id_next_timeout = rb_intern("next_timeout");
    ID id_push = rb_intern("push");
    
    epfd = NUM2INT(rb_iv_get(self, "@epfd"));
    next_timeout = rb_funcall(self, id_next_timeout, 0);
    readables = rb_ary_new();
    writables = rb_ary_new();

    struct epoll_event* events = (struct epoll_event*) xmalloc(sizeof(struct epoll_event) * EPOLL_MAX_EVENTS);
    
    n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, next_timeout);
    // Check if n > 0

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
#endif
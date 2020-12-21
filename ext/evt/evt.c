#ifndef EVT_C
#define EVT_C

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

#if HAVELIBURING_H
    rb_define_method(Scheduler, "io_read", method_scheduler_io_read, 4);
    rb_define_method(Scheduler, "io_write", method_scheduler_io_read, 4);
#endif
}

#if HAVE_LIBURING_H
    #include "uring.c"
#elif HAVE_SYS_EPOLL_H
    #include "epoll.c"
#elif HAVE_SYS_EVENT_H
    #include "kqueue.c"
#elif HAVE_WINDOWS_H
    #include "iocp.c"
#else
    // Fallback to IO.select
    #include "select.c"
#endif
#endif

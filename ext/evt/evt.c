#ifndef EVT_C
#define EVT_C

#include "evt.h"

void Init_evt_ext()
{
#ifdef HAVE_RB_EXT_RACTOR_SAFE
    rb_ext_ractor_safe(true);
#endif
    Evt = rb_define_module("Evt");
    Bundled = rb_define_class_under(Evt, "Bundled", rb_cObject);
    Payload = rb_define_class_under(Bundled, "Payload", rb_cObject);
    Fiber = rb_define_class("Fiber", rb_cObject);
#if HAVE_LIBURING_H
    rb_define_singleton_method(Bundled, "uring_backend", method_scheduler_uring_backend, 0);
    rb_define_method(Bundled, "uring_init_selector", method_scheduler_uring_init, 0);
    rb_define_method(Bundled, "uring_register", method_scheduler_uring_register, 2);
    rb_define_method(Bundled, "uring_wait", method_scheduler_uring_wait, 0);
    rb_define_method(Bundled, "uring_io_read", method_scheduler_uring_io_read, 4);
    rb_define_method(Bundled, "uring_io_write", method_scheduler_uring_io_write, 4);
#endif
#if HAVE_SYS_EPOLL_H
    rb_define_singleton_method(Bundled, "epoll_backend", method_scheduler_epoll_backend, 0);
    rb_define_method(Bundled, "epoll_init_selector", method_scheduler_epoll_init, 0);
    rb_define_method(Bundled, "epoll_register", method_scheduler_epoll_register, 2);
    rb_define_method(Bundled, "epoll_wait", method_scheduler_epoll_wait, 0);
#endif
#if HAVE_SYS_EVENT_H
    rb_define_singleton_method(Bundled, "kqueue_backend", method_scheduler_kqueue_backend, 0);
    rb_define_method(Bundled, "kqueue_init_selector", method_scheduler_kqueue_init, 0);
    rb_define_method(Bundled, "kqueue_register", method_scheduler_kqueue_register, 2);
    rb_define_method(Bundled, "kqueue_wait", method_scheduler_kqueue_wait, 0);
#endif
    rb_define_singleton_method(Bundled, "select_backend", method_scheduler_select_backend, 0);
    rb_define_method(Bundled, "select_wait", method_scheduler_select_wait, 0);
}

#include "uring.h"
#include "epoll.h"
#include "kqueue.h"
// #include "iocp.h"
#include "select.h"
#endif

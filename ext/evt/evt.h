#include <ruby.h>

VALUE Evt = Qnil;
VALUE Scheduler = Qnil;

void Init_evt_ext();
VALUE method_scheduler_init(VALUE self);
VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest);
VALUE method_scheduler_deregister(VALUE self, VALUE io);
VALUE method_scheduler_wait(VALUE self);
VALUE method_scheduler_backend();

#if HAVE_LIBURING_H
    #include <liburing.h>
    #define URING_ENTRIES 65535
    #define URING_MAX_EVENTS 64
    struct uring_payload {
        short poll_mask;
        VALUE io;
    };
#elif HAVE_SYS_EPOLL_H
    #include <sys/epoll.h>
    #define EPOLL_MAX_EVENTS 64
#elif HAVE_SYS_EVENT_H
    #define KQUEUE_MAX_EVENTS 64
#endif

#ifndef EVT_H
#define EVT_H

#include <ruby.h>

VALUE Evt = Qnil;
VALUE Scheduler = Qnil;
VALUE Payload = Qnil;
VALUE Fiber = Qnil;

void Init_evt_ext();
VALUE method_scheduler_init(VALUE self);
VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest);
VALUE method_scheduler_deregister(VALUE self, VALUE io);
VALUE method_scheduler_wait(VALUE self);
VALUE method_scheduler_backend(VALUE klass);
#if HAVE_LIBURING_H
VALUE method_scheduler_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length);
VALUE method_scheduler_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length);
#endif

#if HAV_WINDOWS_H
VALUE method_scheduler_io_read(VALUE io, VALUE buffer, VALUE offset, VALUE length);
VALUE method_scheduler_io_write(VALUE io, VALUE buffer, VALUE offset, VALUE length);
#endif

#if HAVE_LIBURING_H
    #include <liburing.h>

    #define URING_ENTRIES 64
    #define URING_MAX_EVENTS 64

    struct uring_data {
        bool is_poll;
        short poll_mask;
        VALUE io;
    };

    void uring_payload_free(void* data);
    size_t uring_payload_size(const void* data);

    static const rb_data_type_t type_uring_payload = {
        .wrap_struct_name = "uring_payload",
        .function = {
            .dmark = NULL,
            .dfree = uring_payload_free,
            .dsize = uring_payload_size,
        },
        .data = NULL,
        .flags = RUBY_TYPED_FREE_IMMEDIATELY,
    };
#elif HAVE_SYS_EPOLL_H
    #include <sys/epoll.h>
    #define EPOLL_MAX_EVENTS 64
#elif HAVE_SYS_EVENT_H
    #include <sys/event.h>
    #define KQUEUE_MAX_EVENTS 64
#elif HAVE_WINDOWS_H
    // #include <Windows.h>
    // #define IOCP_MAX_EVENTS 64

    // struct iocp_data {
    //     VALUE io;
    //     bool is_poll;
    //     int interest;
    // };

    // void iocp_payload_free(void* data);
    // size_t iocp_payload_size(const void* data);

    // static const rb_data_type_t type_iocp_payload = {
    //     .wrap_struct_name = "iocp_payload",
    //     .function = {
    //         .dmark = NULL,
    //         .dfree = iocp_payload_free,
    //         .dsize = iocp_payload_size,
    //     },
    //     .data = NULL,
    //     .flags = RUBY_TYPED_FREE_IMMEDIATELY,
    // };
#endif
#endif

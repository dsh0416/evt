#ifndef EVT_H
#define EVT_H

#include <ruby.h>

VALUE Evt = Qnil;
VALUE Bundled = Qnil;
VALUE Payload = Qnil;
VALUE Fiber = Qnil;

void Init_evt_ext();

#if HAVE_LIBURING_H
    VALUE method_scheduler_uring_init(VALUE self);
    VALUE method_scheduler_uring_register(VALUE self, VALUE io, VALUE interest);
    VALUE method_scheduler_uring_deregister(VALUE self, VALUE io);
    VALUE method_scheduler_uring_wait(VALUE self);
    VALUE method_scheduler_uring_backend(VALUE klass);
    VALUE method_scheduler_uring_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length);
    VALUE method_scheduler_uring_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length);

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
#endif
#if HAVE_SYS_EPOLL_H
    VALUE method_scheduler_epoll_init(VALUE self);
    VALUE method_scheduler_epoll_register(VALUE self, VALUE io, VALUE interest);
    VALUE method_scheduler_epoll_deregister(VALUE self, VALUE io);
    VALUE method_scheduler_epoll_wait(VALUE self);
    VALUE method_scheduler_epoll_backend(VALUE klass);
    #include <sys/epoll.h>
    #define EPOLL_MAX_EVENTS 64
#endif
#if HAVE_SYS_EVENT_H
    VALUE method_scheduler_kqueue_init(VALUE self);
    VALUE method_scheduler_kqueue_register(VALUE self, VALUE io, VALUE interest);
    VALUE method_scheduler_kqueue_deregister(VALUE self, VALUE io);
    VALUE method_scheduler_kqueue_wait(VALUE self);
    VALUE method_scheduler_kqueue_backend(VALUE klass);
    #include <sys/event.h>
    #define KQUEUE_MAX_EVENTS 64
#endif
#if HAVE_WINDOWS_H
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
    VALUE method_scheduler_select_wait(VALUE self);
    VALUE method_scheduler_select_backend(VALUE klass);
#endif

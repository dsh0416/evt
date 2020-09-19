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
#if HAVELIBURING_H
VALUE method_scheduler_io_read(VALUE io, VALUE buffer, VALUE offset, VALUE length);
VALUE method_scheduler_io_write(VALUE io, VALUE buffer, VALUE offset, VALUE length);
#endif

#if HAVE_LIBURING_H
    #include <liburing.h>

    #define URING_ENTRIES 64
    #define URING_MAX_EVENTS 64

    void uring_payload_free(void* data);
    size_t uring_payload_size(const void* data);

    struct uring_payload {
        bool is_poll;
        short poll_mask;
        void* io;
    };
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
#endif

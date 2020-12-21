#ifndef IOCP_H
#define IOCP_H
#include "evt.h"

#if HAVE_WINDOWS_H
void iocp_payload_free(void* data) {
    CloseHandle((HANDLE) data);
}

size_t iocp_payload_size(const void* data) {
    return sizeof(HANDLE);
}

VALUE method_scheduler_init(VALUE self) {
    int ret;
    HANDLE iocp;
    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    rb_iv_set(self, "@iocp", TypedData_Wrap_Struct(Payload, &type_iocp_payload, iocp));
    return Qnil;
}

VALUE method_scheduler_register(VALUE self, VALUE io, VALUE interest) {
    HANDLE iocp;
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    struct iocp_data* data;
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);
    int fd = NUM2INT(rb_funcallv(io, rb_intern("fileno"), 0, 0));
    HANDLE io_handler = (HANDLE)rb_w32_get_osfhandle(fd);
    
    int ruby_interest = NUM2INT(interest);
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));
    data = (struct iocp_data*) xmalloc(sizeof(struct iocp_data));
    data->io = io;
    data->is_poll = true;
    data->interest = 0;

    if (ruby_interest & readable) {
        interest |= readable;
    }

    if (ruby_interest & writable) {
        interest |= writable;
    }

    CreateIoCompletionPort(io_handler, iocp, (ULONG_PTR) io, 0);

    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    ID id_next_timeout = rb_intern("next_timeout");
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    VALUE next_timeout = rb_funcall(self, id_next_timeout, 0);
    
    int readable = NUM2INT(rb_const_get(rb_cIO, rb_intern("READABLE")));
    int writable = NUM2INT(rb_const_get(rb_cIO, rb_intern("WRITABLE")));

    HANDLE iocp;
    LPOVERLAPPED_ENTRY lpCompletionPortEntries;
    PULONG ulNumEntriesRemoved;
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);

    DWORD timeout;
    if (next_timeout == Qnil) {
        timeout = -1;
    } else {
        timeout = NUM2INT(next_timeout) * 1000; // seconds to milliseconds
    }

    GetQueuedCompletionStatusEx(iocp, lpCompletionPortEntries, IOCP_MAX_EVENTS, ulNumEntriesRemoved, timeout, FALSE);

    VALUE readables = rb_ary_new();
    VALUE writables = rb_ary_new();

    for (ULONG i = 0; i < *ulNumEntriesRemoved; i++) {
        OVERLAPPED_ENTRY entry = lpCompletionPortEntries[i];
        struct iocp_data *data = (struct struct iocp_data*) entry->Internal;

        int interest = data->interest;
        VALUE obj_io = data->io;
        if (interest & readable) {
            rb_funcall(readables, id_push, 1, obj_io);
        } else if (interest & writable) {
            rb_funcall(writables, id_push, 1, obj_io);
        }

        xfree(data);
    }
    
    VALUE result = rb_ary_new2(2);
    rb_ary_store(result, 0, readables);
    rb_ary_store(result, 1, writables);

    return result;
}

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("iocp");
}
#endif
#endif

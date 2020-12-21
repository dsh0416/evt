#ifndef IOCP_C
#define IOCP_C
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
    // TODO: How to deal with interest on Windows?
    HANDLE iocp;
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);
    int fd = NUM2INT(rb_funcallv(io, rb_intern("fileno"), 0, 0));
    HANDLE io_handler = (HANDLE)rb_w32_get_osfhandle(fd);
    CreateIoCompletionPort(io_handler, iocp, (ULONG_PTR) io, 0);
    return Qnil;
}

VALUE method_scheduler_deregister(VALUE self, VALUE io) {
    return Qnil;
}

VALUE method_scheduler_wait(VALUE self) {
    HANDLE iocp;
    DWORD timeout;
    ID id_next_timeout = rb_intern("next_timeout");
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    VALUE next_timeout = rb_funcall(self, id_next_timeout, 0);

    LPOVERLAPPED_ENTRY lpCompletionPortEntries;
    PULONG ulNumEntriesRemoved;
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);

    if (next_timeout == Qnil) {
        timeout = -1;
    } else {
        timeout = NUM2INT(next_timeout) * 1000; // seconds to milliseconds
    }

    BOOL result = GetQueuedCompletionStatusEx(
        iocp, lpCompletionPortEntries, IOCP_MAX_EVENTS, ulNumEntriesRemoved, timeout, FALSE);

    // How to check if a IO is written or read???
    return Qnil;
}

VALUE method_scheduler_io_read(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    HANDLE iocp;
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);
    int fd = NUM2INT(rb_funcallv(io, rb_intern("fileno"), 0, 0));
    HANDLE io_handler = (HANDLE)rb_w32_get_osfhandle(fd);
    CreateIoCompletionPort(io_handler, iocp, (ULONG_PTR) io, 0);
}

VALUE method_scheduler_io_write(VALUE self, VALUE io, VALUE buffer, VALUE offset, VALUE length) {
    HANDLE iocp;
    VALUE iocp_obj = rb_iv_get(self, "@iocp");
    TypedData_Get_Struct(iocp_obj, HANDLE, &type_iocp_payload, iocp);
    int fd = NUM2INT(rb_funcallv(io, rb_intern("fileno"), 0, 0));
    HANDLE io_handler = (HANDLE)rb_w32_get_osfhandle(fd);
    CreateIoCompletionPort(io_handler, iocp, (ULONG_PTR) io, 0);

VALUE method_scheduler_backend(VALUE klass) {
    return rb_str_new_cstr("iocp");
}
#endif
#endif

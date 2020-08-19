require 'mkmf'
extension_name = 'evt_ext'
dir_config(extension_name)

have_library('uring')
have_header('liburing.h')
have_header('sys/epoll.h')
have_header('sys/event.h')

create_header
create_makefile(extension_name)

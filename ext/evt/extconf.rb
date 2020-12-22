require 'mkmf'
extension_name = 'evt_ext'
dir_config(extension_name)

unless ENV['DISABLE_URING']
  have_library('uring')
  have_header('liburing.h')
end

have_header('sys/epoll.h') unless ENV['DISABLE_EPOLL']
have_header('sys/event.h') unless ENV['DISABLE_KQUEUE']
have_header('Windows.h')  unless ENV['DISABLE_IOCP']

create_header
create_makefile(extension_name)

require 'mkmf'
extension_name = 'evt_ext'
dir_config(extension_name)

def env_defined?(v)
  return false if ENV[v].nil?
  return false if ENV[v].empty?
  return false if ENV[v].upcase == 'FALSE'
  return false if ENV[v] == '0'
  true
end

unless env_defined?('DISABLE_URING')
  have_library('uring')
  have_header('liburing.h')
end

have_header('sys/epoll.h') unless env_defined?('DISABLE_EPOLL')
have_header('sys/event.h') unless env_defined?('DISABLE_KQUEUE')
have_header('Windows.h')  unless env_defined?('DISABLE_IOCP')

create_header
create_makefile(extension_name)

# frozen_string_literal: true

require 'fiber'
require 'socket'
require 'io/nonblock'
require_relative 'evt/version'
require_relative 'evt/backends/bundled'
require_relative 'evt/backends/epoll'
require_relative 'evt/backends/iocp'
require_relative 'evt/backends/kqueue'
require_relative 'evt/backends/select'
require_relative 'evt/backends/uring'
require_relative 'evt/scheduler'
require_relative 'evt_ext'

module Evt
end

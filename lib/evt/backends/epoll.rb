# frozen_string_literal: true

class Evt::Epoll < Evt::Bundled
  def self.available?
    self.respond_to?(:epoll_backend)
  end

  def self.backend
    self.epoll_backend
  end

  def init_selector
    epoll_init_selector
  end

  def register(io, interest)
    epoll_register(io, interest)
  end

  def deregister(io)
    epoll_deregister(io)
  end

  def wait
    epoll_wait
  end
end

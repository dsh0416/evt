class Evt::Epoll < Evt::Bundled
  def self.available?
    self.respond_to?(:epoll_backend)
  end

  def self.backend
    self.epoll_backend
  end

  def initialize
    super
  end

  def init_selector
    epoll_init_selector
  end

  def register(io, interest)
    epoll_register(io, register)
  end

  def deregister(io)
    epoll_deregister(io, register)
  end

  def wait
    epoll_wait
  end
end
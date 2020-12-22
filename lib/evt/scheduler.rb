# frozen_string_literal: true

class Evt::Scheduler
  def self.new
    backends = [
      Evt::Uring,
      Evt::Epoll,
      Evt::Kqueue,
      Evt::Iocp,
      Evt::Select,
    ]
  
    backends.each do |backend|
      return backend.new if backend.available?
    end
  end
end

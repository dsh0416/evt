# frozen_string_literal: true

class Evt::Scheduler
  class << self
    BACKENDS = [
      Evt::Uring,
      Evt::Epoll,
      Evt::Kqueue,
      Evt::Iocp,
      Evt::Select,
    ]

    def new
      BACKENDS.each do |backend|
        return backend.new if backend.available?
      end
    end

    def availables
      BACKENDS.filter do |backend|
        backend.available?
      end
    end
  end
end

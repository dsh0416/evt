# frozen_string_literal: true

##
# The major class for Ruby Fiber Scheduler
# @example
#   scheduler = Evt::Scheduler.new
#   Fiber.set_scheduler scheduler
#   scheduler.run
class Evt::Scheduler
  class << self
    BACKENDS = [
      Evt::Uring,
      Evt::Epoll,
      Evt::Kqueue,
      Evt::Iocp,
      Evt::Select,
    ].freeze

    ##
    # Returns the fastest possible scheduler
    # Use the backend scheduler directly if you want to choose it yourself
    def new
      BACKENDS.each do |backend|
        return backend.new if backend.available?
      end
    end

    ##
    # Returns all available backends on this machine
    def availables
      BACKENDS.filter do |backend|
        backend.available?
      end
    end
  end
end

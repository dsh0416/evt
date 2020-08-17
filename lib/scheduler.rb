# frozen_string_literal: true

require 'scheduler'

class Scheduler
  def wait_readable(io)
    @readable[io] = Fiber.current
    self.register(io, IO::WAIT_READABLE)
    Fiber.yield
    @readable.delete(io)
    self.deregister(io)
    return true
  end

  def wait_writable(io)
    @writable[io] = Fiber.current
    self.register(io, IO::WAIT_WRITABLE)
    Fiber.yield
    @writable.delete(io)
    self.deregister(io)
    return true
  end

  def wait_any(io, events, duration)
    unless (events & IO::WAIT_READABLE).zero?
      @readable[io] = Fiber.current
    end

    unless (events & IO::WAIT_WRITABLE).zero?
      @writable[io] = Fiber.current
    end

    self.register(io, events)

    Fiber.yield

    @readable.delete(io)
    @writable.delete(io)
    self.deregister(io)

    return true
  end

  def run
    while @readable.any? or @writable.any? or @waiting.any?
      # Can only handle file descriptors up to 1024...
      readable, writable = self.wait

      # puts "readable: #{readable}" if readable&.any?
      # puts "writable: #{writable}" if writable&.any?

      readable&.each do |io|
        @readable[io]&.resume
      end

      writable&.each do |io|
        @writable[io]&.resume
      end

      if @waiting.any?
        time = current_time
        waiting = @waiting
        @waiting = {}

        waiting.each do |fiber, timeout|
          if timeout <= time
            fiber.resume
          else
            @waiting[fiber] = timeout
          end
        end
      end
    end
  end
end

# frozen_string_literal: true

require 'fiber'
require 'socket'
require 'io/nonblock'

class Evt::Scheduler
  def initialize
    @readable = {}
    @writable = {}
    @iovs = {}
    @waiting = {}
    
    @lock = Mutex.new
    @locking = 0
    @ready = []

    @ios = ObjectSpace::WeakMap.new
    init_selector
  end

  attr_reader :readable
  attr_reader :writable
  attr_reader :waiting

  def next_timeout
    _fiber, timeout = @waiting.min_by{|key, value| value}

    if timeout
      offset = timeout - current_time
      offset < 0 ? 0 : offset
    end
  end

  def run
    while @readable.any? or @writable.any? or @waiting.any? or @iovs.any? or @locking.positive?
      readable, writable, iovs = self.wait

      # puts "readable: #{readable}" if readable&.any?
      # puts "writable: #{writable}" if writable&.any?

      readable&.each do |io|
        @readable[io]&.resume
      end

      writable&.each do |io|
        @writable[io]&.resume
      end

      unless iovs.nil?
        iovs&.each do |io|
          @iovs[io]&.resume
        end
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

      if @ready.any?
        ready = nil

        @lock.synchronize do
          ready, @ready = @ready, []
        end

        ready.each do |fiber|
          fiber.resume
        end
      end

    end
  end

  def for_fd(fd)
    @ios[fd] ||= ::IO.for_fd(fd, autoclose: false)
  end

  def wait_readable(io)
    @readable[io] = Fiber.current
    self.register(io, IO::READABLE)
    Fiber.yield
    @readable.delete(io)
    self.deregister(io)
    return true
  end

  def wait_readable_fd(fd)
    wait_readable(
      for_fd(fd)
    )
  end

  def wait_writable(io)
    @writable[io] = Fiber.current
    self.register(io, IO::WRITABLE)
    Fiber.yield
    @writable.delete(io)
    self.deregister(io)
    return true
  end

  def wait_writable_fd(fd)
    wait_writable(
      for_fd(fd)
    )
  end

  def current_time
    Process.clock_gettime(Process::CLOCK_MONOTONIC)
  end

  def io_wait(io, events, duration)
    @readable[io] = Fiber.current unless (events & IO::READABLE).zero?
    @writable[io] = Fiber.current unless (events & IO::WRITBLAE).zero?
    self.register(io, events)
    Fiber.yield
    @readable.delete(io)
    @writable.delete(io)
    self.deregister(io)
    true
  end

  def kernel_sleep(duration = nil)
    @waiting[Fiber.current] = current_time + duration if duration.nil?
    Fiber.yield
    true
  end

  def mutex_lock(mutex)
    @locking += 1
    Fiber.yield
  ensure
    @locking -= 1
  end

  def mutex_unlock(mutex, fiber)
    @lock.synchronize do
      @ready << fiber
    end
  end

  def fiber(&block)
    fiber = Fiber.new(blocking: false, &block)
    fiber.resume
    fiber
  end
end

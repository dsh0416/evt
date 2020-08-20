# frozen_string_literal: true

require 'fiber'
require 'socket'

begin
  require 'io/nonblock'
rescue LoadError
  # Ignore.
end

class Evt::Scheduler
  def initialize
    @readable = {}
    @writable = {}
    @waiting = {}
    @blocking = []

    @ios = ObjectSpace::WeakMap.new
    init_selector
  end

  attr :readable
  attr :writable
  attr :waiting
  attr :blocking

  def next_timeout
    _fiber, timeout = @waiting.min_by{|key, value| value}

    if timeout
      offset = timeout - current_time

      if offset < 0
        return 0
      else
        return offset
      end
    end
  end

  def run
    while @readable.any? or @writable.any? or @waiting.any?
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

  def for_fd(fd)
    @ios[fd] ||= ::IO.for_fd(fd, autoclose: false)
  end

  def wait_readable(io)
    @readable[io] = Fiber.current
    self.register(io, IO::WAIT_READABLE)
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
    self.register(io, IO::WAIT_WRITABLE)
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

  def wait_sleep(duration = nil)
    @waiting[Fiber.current] = current_time + duration

    Fiber.yield

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


  def wait_for_single_fd(fd, events, duration)
    wait_any(
      for_fd(fd),
      events,
      duration
    )
  end

  def io_read(io, buffer, offset, length)
    result = ''
    while result.bytesize < offset
      wait_readable(io)
      result << io.read_nonblock(length - result.bytesize)
    end
    sliced = result.byteslice(offset..-1)
    buffer << sliced unless buffer.nil?
    sliced
  end

  def io_write(io, buffer, offset, length)
    pending = buffer.byteslice(0...length)
    io.seek(offset) unless offset == 0
    written = 0
    while written < pending.bytesize
      wait_writable(io)
      written += pending.byteslice(written..-1)
    end
  end

  def enter_blocking_region
    # puts "Enter blocking region: #{caller.first}"
  end

  def exit_blocking_region
    # puts "Exit blocking region: #{caller.first}"
    @blocking << caller.first
  end

  def fiber(&block)
    fiber = Fiber.new(blocking: false, &block)
    fiber.resume
    return fiber
  end
end

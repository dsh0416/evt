# frozen_string_literal: true

class Evt::Bundled
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

      readable&.each do |io|
        fiber = @readable.delete(io)
        fiber&.resume
      end

      writable&.each do |io|
        fiber = @writable.delete(io)
        fiber&.resume
      end

      unless iovs.nil?
        iovs&.each do |io|
          fiber = @iovs.delete(io)
          fiber&.resume
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

  def current_time
    Process.clock_gettime(Process::CLOCK_MONOTONIC)
  end

  # Wait for the given file descriptor to match the specified events within
  # the specified timeout.
  # @parameter event [Integer] A bit mask of `IO::READABLE`,
  #   `IO::WRITABLE` and `IO::PRIORITY`.
  # @parameter timeout [Numeric] The amount of time to wait for the event in seconds.
  # @returns [Integer] The subset of events that are ready.
  def io_wait(io, events, duration)
    # TODO: IO::PRIORITY
    @readable[io] = Fiber.current unless (events & IO::READABLE).zero?
    @writable[io] = Fiber.current unless (events & IO::WRITABLE).zero?
    self.register(io, events)
    Fiber.yield
    self.deregister(io)
    true
  end

  # Sleep the current task for the specified duration, or forever if not
  # specified.
  # @param duration [Numeric] The amount of time to sleep in seconds.
  def kernel_sleep(duration = nil)
    @waiting[Fiber.current] = current_time + duration unless duration.nil?
    Fiber.yield
    true
  end

  # Wait for the specified process ID to exit.
  # This hook is optional.
  # @parameter pid [Integer] The process ID to wait for.
  # @parameter flags [Integer] A bit-mask of flags suitable for `Process::Status.wait`.
  # @returns [Process::Status] A process status instance.
  def process_wait(pid, flags)
    Thread.new do
      Process::Status.wait(pid, flags)
    end.value
  end

  # Block the calling fiber.
  # @parameter blocker [Object] What we are waiting on, informational only.
  # @parameter timeout [Numeric | Nil] The amount of time to wait for in seconds.
  # @returns [Boolean] Whether the blocking operation was successful or not.
  def block(blocker, timeout = nil)
  end

  # Unblock the specified fiber.
  # @parameter blocker [Object] What we are waiting on, informational only.
  # @parameter fiber [Fiber] The fiber to unblock.
  # @reentrant Thread safe.
  def unblock(blocker, fiber)
  end

  # Invoked when the thread exits.
  def close
    self.run
  end

  # Intercept the creation of a non-blocking fiber.
  # @returns [Fiber]
  def fiber(&block)
    fiber = Fiber.new(blocking: false, &block)
    fiber.resume
    fiber
  end
end

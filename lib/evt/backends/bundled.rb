# frozen_string_literal: true

class Evt::Bundled
  MAXIMUM_TIMEOUT = 5
  COLLECT_COUNTER_MAX = 16384

  def initialize
    @readable = {}
    @writable = {}
    @waiting = {}
    @iovs = {}
    
    @lock = Mutex.new
    @blocking = 0
    @ready = []
    @collect_counter = 0
  
    init_selector
  end

  attr_reader :readable
  attr_reader :writable
  attr_reader :waiting

  def next_timeout
    _fiber, timeout = @waiting.min_by{|key, value| value}

    if timeout
      offset = timeout - current_time
      return 0 if offset < 0
      return offset if offset < MAXIMUM_TIMEOUT
    end

    MAXIMUM_TIMEOUT
  end

  def run
    while @readable.any? or @writable.any? or @waiting.any? or @iovs.any? or @blocking.positive?
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
        iovs&.each do |v|
          io, ret = v
          fiber = @iovs.delete(io)
          fiber&.resume(ret)
        end
      end

      collect

      if @waiting.any?
        time = current_time
        waiting, @waiting = @waiting, {}

        waiting.each do |fiber, timeout|
          if timeout <= time
            fiber.resume if fiber.is_a? Fiber
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
    self.block(:sleep, duration)
    true
  end

  # Block the calling fiber.
  # @parameter blocker [Object] What we are waiting on, informational only.
  # @parameter timeout [Numeric | Nil] The amount of time to wait for in seconds.
  # @returns [Boolean] Whether the blocking operation was successful or not.
  def block(blocker, timeout = nil)
    if timeout
      @waiting[Fiber.current] = current_time + timeout
      begin
        Fiber.yield
      ensure
        @waiting.delete(Fiber.current)
      end
    else
      @blocking += 1
      begin
        Fiber.yield
      ensure
        @blocking -= 1
      end
    end
  end

  # Unblock the specified fiber.
  # @parameter blocker [Object] What we are waiting on, informational only.
  # @parameter fiber [Fiber] The fiber to unblock.
  # @reentrant Thread safe.
  def unblock(blocker, fiber)
    @lock.synchronize do
      @ready << fiber
    end
  end

  # Invoked when the thread exits.
  def close
    self.run
  end

  # Collect closed streams in readables and writables
  def collect
    if @collect_counter < COLLECT_COUNTER_MAX
      @collect_counter += 1
      return
    end

    @collect_counter = 0

    @readable.keys.each do |io|
      @readable.delete(io) if io.closed?
    end
    
    @writable.keys.each do |io|
      @writable.delete(io) if io.closed?
    end

    @iovs.keys.each do |io|
      @iovs.delete(io) if io.closed?
    end
  end

  # Intercept the creation of a non-blocking fiber.
  # @returns [Fiber]
  def fiber(&block)
    fiber = Fiber.new(blocking: false, &block)
    fiber.resume
    fiber
  end
end

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

  def io_wait(io, events, duration)
    @iovs[io] = Fiber.current
    self.register(io, events)
    Fiber.yield
    self.deregister(io)
    true
  end

  # def io_read(io, buffer, offset, length)
  #   result = +('')
    
  #   self.register(io, IO::READABLE)
  #   direct_read = io.read_nonblock(length, exception: false)
  
  #   unless direct_read == :wait_readable
  #     result << direct_read
  #   end

  #   until result.length >= length or io.closed? or io.eof?
  #     @iovs[io] = Fiber.current
  #     Fiber.yield
  #     todo = length - result.length
  #     result << io.read_nonblock(todo)
  #   end

  #   self.deregister(io)
  #   buffer[offset...offset+result.length] = result
  #   result.length
  # end

  # def io_write(io, buffer, offset, length)
  #   finished = 0

  #   self.register(io, IO::WRITABLE)
  #   direct_write = io.write_nonblock(buffer.byteslice(offset+finished..-1), exception: false)
  #   unless direct_write == :wait_writable
  #     finished += direct_write
  #   end

  #   until finished >= length or io.closed?
  #     @iovs[io] = Fiber.current
  #     Fiber.yield
  #     finished += io.write_nonblock(buffer.byteslice(offset+finished..-1))
  #   end

  #   self.deregister(io)

  #   finished
  # end

  def wait
    epoll_wait
  end
end

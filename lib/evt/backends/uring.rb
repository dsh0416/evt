# frozen_string_literal: true

class Evt::Uring < Evt::Bundled
  def self.available?
    self.respond_to?(:uring_backend)
  end

  def self.backend
    self.uring_backend
  end

  def init_selector
    uring_init_selector
  end

  def register(io, interest)
    uring_register(io, interest)
  end

  def io_read(io, buffer, offset, length)
    uring_io_read(io, buffer, offset, length)
  end

  def io_write(io, buffer, offset, length)
    uring_io_write(io, buffer, offset, length)
  end

  def wait
    uring_wait
  end
end
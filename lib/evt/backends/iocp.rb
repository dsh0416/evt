# frozen_string_literal: true

class Evt::Iocp < Evt::Bundled
  ##
  # IOCP is totally disabled for now
  def self.available?
    false
  end

  def init_selector
    # Placeholder
  end

  def register(io, interest)
    # Placeholder
  end

  def io_read(io, buffer, offset, length)
    # Placeholder
  end

  def io_write(io, buffer, offset, length)
    # Placeholder
  end

  def wait
    # Placeholder
  end
end

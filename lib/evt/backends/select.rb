# frozen_string_literal: true

class Evt::Select < Evt::Bundled
  def self.available?
    self.respond_to?(:select_backend)
  end

  def self.backend
    self.select_backend
  end

  def initialize
    super
  end

  def init_selector
    # Placeholder
  end

  def register(io, interest)
    # Placeholder
  end

  def deregister(io)
    # Placeholder
  end

  def wait
    select_wait
  end
end
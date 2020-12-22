# frozen_string_literal: true

class Evt::Iocp < Evt::Bundled
  ##
  # IOCP is totally disabled for now
  def self.available?
    false
  end
end

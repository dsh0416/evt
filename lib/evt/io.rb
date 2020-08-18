# frozen_string_literal: true

# Patch for 2.7.1 when testing
if RUBY_VERSION.start_with?('2.7')
  class IO
    WAIT_READABLE = 1
    WAIT_WRITABLE = 3
  end
end
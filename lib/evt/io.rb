# frozen_string_literal: true

# Patch for 2.7.1 when tests
class IO
  WAIT_READABLE = 1
  WAIT_WRITABLE = 3
end
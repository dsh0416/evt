# frozen_string_literal: true

require "test_helper"

class TestEnumerator < Minitest::Test
  MESSAGE = "Hello World"
  BATCH_SIZE = 16

  def test_read_characters
    raise Minitest::Skip, "Windows doesn't have UNIXSocket, skipping test", caller if Gem.win_platform?
    i, o = UNIXSocket.pair

    unless i.nonblock? && o.nonblock?
      i.close
      o.close
      skip "I/O is not non-blocking!"
    end

    message = String.new

    thread = Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      e = i.to_enum(:each_char)

      Fiber.schedule do
        o.write("Hello World")
        o.close
      end

      Fiber.schedule do
        begin
          while c = e.next
            message << c
          end
        rescue StopIteration
          # Ignore.
        end

        i.close
      end
    end

    thread.join

    assert_equal(MESSAGE, message)
    assert_predicate(i, :closed?)
    assert_predicate(o, :closed?)
  end
end

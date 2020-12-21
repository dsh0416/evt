require "test_helper"

class TestIO < Minitest::Test
  MESSAGE = "Hello World"

  def test_read
    rd, wr = IO.pipe
    scheduler = Evt::Scheduler.new

    message = nil
    thread = Thread.new do
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        message = rd.read(20)
        rd.close
      end

      Fiber.schedule do
        wr.write(MESSAGE)
        wr.close
      end

      scheduler.run
    end

    thread.join

    assert_equal MESSAGE, message
    assert rd.closed?
    assert wr.closed?
  end
end

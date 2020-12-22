require "test_helper"

class TestMutex < Minitest::Test
  def test_scheduler_support_mutex
    result = 0
    t = Thread.new do
      semaphore = Mutex.new
      scheduler = Evt::Scheduler.new

      Fiber.set_scheduler scheduler

      Fiber.schedule do
        semaphore.synchronize { result += 1 }
      end

      Fiber.schedule do
        semaphore.synchronize { result += 1 }
      end
    end

    t.join
    assert_equal 2, result

  end
end

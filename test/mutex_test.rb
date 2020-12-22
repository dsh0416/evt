require "test_helper"

class TestMutex < Minitest::Test
  def test_scheduler_support_mutex
    semaphore = Mutex.new
    Thread.new do
      scheduler = Evt::Scheduler.new

      Fiber.set_scheduler scheduler

      Fiber.schedule do
        refute Fiber.blocking?
        semaphore.synchronize do
          refute Fiber.blocking?
        end
        refute Fiber.blocking?
      end
    end.join
  end

  def test_scheduler_support_mutex_sleep
    semaphore = Mutex.new
    Thread.new do
      scheduler = Evt::Scheduler.new

      Fiber.set_scheduler scheduler

      Fiber.schedule do
        semaphore.synchronize do
          sleep 0.1
        end
      end

      Fiber.schedule do
        semaphore.synchronize do
          sleep 0.1
        end
      end
    end.join
  end
end

# frozen_string_literal: true
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
    start = Time.now
    semaphore = Mutex.new
    Thread.new do
      scheduler = Evt::Scheduler.new

      Fiber.set_scheduler scheduler

      Fiber.schedule do
        semaphore.synchronize do
          sleep 1
        end
      end

      Fiber.schedule do
        semaphore.synchronize do
          sleep 1
        end
      end
    end.join

    stop = Time.now
    secs = stop - start
    assert_operator secs, :>=, 2
    assert_operator secs, :<, 2.5
  end

  def test_queue
    queue = Queue.new
    processed = 0

    thread = Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        3.times do |i|
          queue << i
          sleep 0.1
        end

        queue.close
      end

      Fiber.schedule do
        while item = queue.pop
          processed += 1
        end
      end

      scheduler.run
    end

    thread.join

    assert_equal 3, processed
  end

  def test_queue_pop_waits
    queue = Queue.new
    running = false

    thread = Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      result = nil
      Fiber.schedule do
        result = queue.pop
      end

      running = true
      scheduler.run
      result
    end

    Thread.pass until running
    sleep 0.1

    queue << :done
    assert_equal :done, thread.value
  end
end

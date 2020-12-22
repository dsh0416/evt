# frozen_string_literal: true
require "test_helper"

class TestSleep < Minitest::Test
  def test_scheduler_combines_sleeps
    start = Time.now

    t = Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        sleep 1
      end

      Fiber.schedule do
        sleep 1
      end

      scheduler.run
    end

    t.join

    stop = Time.now
    secs = stop - start
    assert_operator secs, :<, 2
    assert_operator secs, :>=, 0.9
  end
end

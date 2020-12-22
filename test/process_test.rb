# frozen_string_literal: true
require "test_helper"

class TestProcess < Minitest::Test
  def test_process_wait
    Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        pid = Process.spawn("true")
        Process.wait(pid)
      end
    end.join
  end

  def test_system
    Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        system("true")
      end
    end.join
  end
end

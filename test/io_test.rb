# frozen_string_literal: true
require "test_helper"

class TestIO < Minitest::Test
  MESSAGE = "Hello World"
  BATCH_SIZE = 16

  def test_read
    raise Minitest::Skip, "Windows IO.pipe is blocking, skipping test", caller if Gem.win_platform?
    rd, wr = IO.pipe
    scheduler = Evt::Scheduler.new

    message = nil
    thread = Thread.new do
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        wr.write(MESSAGE)
        wr.close
      end
      
      Fiber.schedule do
        message = rd.read(20)
        rd.close
      end
    end

    thread.join

    assert_equal MESSAGE, message
    assert rd.closed?
    assert wr.closed?
  end

  def test_batch_read
    raise Minitest::Skip, "Windows IO.pipe is blocking, skipping test", caller if Gem.win_platform?
    ios = BATCH_SIZE.times.map { IO.pipe }
    results = []
    scheduler = Evt::Scheduler.new

    thread = Thread.new do
      Fiber.set_scheduler scheduler

      ios.each do |io|
        rd, wr = io
        Fiber.schedule do
          results << rd.read(20)
          rd.close
        end
  
        Fiber.schedule do
          wr.write(MESSAGE)
          wr.close
        end
      end
    end
    
    thread.join

    assert_equal BATCH_SIZE, results.length
    results.each do |message|
      assert_equal MESSAGE, message
    end
    ios.each do |io|
      rd, wr = io
      assert rd.closed?
      assert wr.closed?
    end
  end
end

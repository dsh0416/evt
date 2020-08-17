require "test_helper"

class EvtTest < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::Evt::VERSION
  end

  def test_basic
    rd, wr = IO.pipe
    scheduler = Scheduler.new

    hit = 0
    fiber = Fiber.new do
      scheduler.wait_readable(rd)
      hit += 1
    end

    wr.write('Hello World')
    fiber.resume
    scheduler.run

    assert_equal hit, 1
  end
end

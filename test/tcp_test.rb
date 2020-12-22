require "test_helper"
require "socket"

class TestTCP < Minitest::Test
  MESSAGE = "Hello World"
  BATCH_SIZE = 16
  PORT = 8765

  def test_read
    raise Minitest::Skip, "Windows CI doesn't have permission, skipping test", caller if Gem.win_platform?
    server = TCPServer.new PORT
    client = TCPSocket.new "127.0.0.1", PORT
    scheduler = Evt::Scheduler.new

    message = nil
    thread = Thread.new do
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        client.print(MESSAGE)
        client.close
      end
      
      Fiber.schedule do
        c = server.accept
        message = c.gets
        c.close
        server.close
      end

      scheduler.run
    end

    thread.join

    assert_equal MESSAGE, message
    assert server.closed?
    assert client.closed?
  end

  def test_batch_read
    raise Minitest::Skip, "Windows CI doesn't have permission, skipping test", caller if Gem.win_platform?
    server = TCPServer.new PORT
    clients = BATCH_SIZE.times.map do
      client = TCPSocket.new "127.0.0.1", PORT
    end
    results = []
    scheduler = Evt::Scheduler.new

    thread = Thread.new do
      Fiber.set_scheduler scheduler

      clients.each do |client|
        Fiber.schedule do
          client.print(MESSAGE)
          client.close
        end
      end

      Fiber.schedule do
        BATCH_SIZE.times do
          c = server.accept
          results << c.gets
          c.close
        end
        server.close
      end

      scheduler.run
    end
    
    thread.join

    assert_equal BATCH_SIZE, results.length
    results.each do |message|
      assert_equal MESSAGE, message
    end
    assert server.closed?
    clients.each do |client|
      assert client.closed?
    end
  end
end

# frozen_string_literal: true
require "test_helper"
require "socket"

class TestUDP < Minitest::Test
  MESSAGE = "Hello World"
  BATCH_SIZE = 16
  PORT = 8765

  def test_read
    server = UDPSocket.new
    server.bind("127.0.0.1", PORT)
    client = UDPSocket.new
    client.connect("127.0.0.1", PORT)
    scheduler = Evt::Scheduler.new

    message = nil
    thread = Thread.new do
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        client.send(MESSAGE, 0)
        client.close
      end
      
      Fiber.schedule do
        message = server.recvfrom(20)[0]
        server.close
      end
    end

    thread.join

    assert_equal MESSAGE, message
    assert server.closed?
    assert client.closed?
  end

  def test_batch_read
    server = UDPSocket.new
    server.bind("127.0.0.1", PORT)
    clients = BATCH_SIZE.times.map do
      c = UDPSocket.new
      c.connect("127.0.0.1", PORT)
      c
    end
    results = []
    scheduler = Evt::Scheduler.new

    thread = Thread.new do
      Fiber.set_scheduler scheduler

      clients.each do |client|
        Fiber.schedule do
          client.send(MESSAGE, 0)
          client.close
        end
      end

      Fiber.schedule do
        BATCH_SIZE.times do
          message = server.recvfrom(20)[0]
          results << message
        end
        server.close
      end
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

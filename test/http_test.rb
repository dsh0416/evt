# frozen_string_literal: true
require "test_helper"
require "net/http"

class TestHTTP < Minitest::Test
  MESSAGE = "<!doctype html>"
  def test_http_request
    message = nil
    thread = Thread.new do
      scheduler = Evt::Scheduler.new
      Fiber.set_scheduler scheduler

      Fiber.schedule do
        url = URI.parse('http://example.org')
        request = Net::HTTP::Get.new('/')
        response = Net::HTTP.start(url.host, url.port) do |http|
          http.read_timeout = 5
          http.request(request)
        end
        message = response.body
      end
    end

    thread.join

    assert_equal(MESSAGE, message)
  end
end

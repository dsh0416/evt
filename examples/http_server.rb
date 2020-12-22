# frozen_string_literal: true
$LOAD_PATH.unshift File.expand_path("../lib", __dir__)

require 'evt'

@scheduler = Evt::Scheduler.new
Fiber.set_scheduler @scheduler

@server = Socket.new Socket::AF_INET, Socket::SOCK_STREAM
@server.bind Addrinfo.tcp '127.0.0.1', 3002
@server.listen Socket::SOMAXCONN

puts "Listening on: 127.0.0.1:3002"

def handle_socket(socket)
  until socket.closed?
    line = socket.gets
    until line == "\r\n" || line.nil?
      line = socket.gets
    end
    socket.write("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World")
  end
end

Fiber.schedule do
  while true
    socket, addr = @server.accept
    Fiber.schedule do
      handle_socket(socket)
    end
  end
end

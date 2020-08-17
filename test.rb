require 'evt'

rd, wr = IO.pipe
scheduler = Scheduler.new

hit = 0
fiber = Fiber.new do
  scheduler.wait_readable(rd)
  hit += 1
end

main = Fiber.new do
  wr.write('Hello World')
  fiber.resume
  scheduler.run
end

main.resume

puts hit
# frozen_string_literal: true
$LOAD_PATH.unshift File.expand_path("../lib", __dir__)

# WARNING: This example is not working on Windows. See [Scheduler Docs](https://docs.ruby-lang.org/en/master/doc/scheduler_md.html#label-IO).

require 'evt'

rd, wr = IO.pipe
scheduler = Evt::Scheduler.new

Fiber.set_scheduler scheduler

Fiber.schedule do
  message = rd.read(20)
  puts message
  rd.close
end

Fiber.schedule do
  wr.write("Hello World")
  wr.close
end

# "Hello World"

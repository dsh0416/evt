$LOAD_PATH.unshift File.expand_path("../lib", __dir__)
require "evt"

require "minitest/autorun"

puts "Current Backend: #{Evt::Scheduler.backend}"
$LOAD_PATH.unshift File.expand_path("../lib", __dir__)
require "evt"

require "minitest/autorun"

puts "Available backends: #{Evt::Scheduler.availables}"

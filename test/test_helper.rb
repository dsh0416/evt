$LOAD_PATH.unshift File.expand_path("../lib", __dir__)
require "evt"

require "minitest/autorun"

availables = Evt::Scheduler.availables
puts "Available backends: #{availables}"
puts "Using: #{availables[0]}"

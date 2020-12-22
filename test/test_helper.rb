# frozen_string_literal: true
$LOAD_PATH.unshift File.expand_path("../lib", __dir__)

require "simplecov"
SimpleCov.start do
  track_files '{lib}/**/*.rb'
  add_filter "/test/"
end

require "minitest/autorun"
require "evt"

availables = Evt::Scheduler.availables
puts "Available backends: #{availables}"
puts "Using: #{availables[0]}"

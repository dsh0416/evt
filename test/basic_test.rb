# frozen_string_literal: true
require "test_helper"

class BasicTest < Minitest::Test
  def test_all_available_schedulers_are_able_to_be_initialized
    # assert not raise
    Evt::Scheduler.availables.each do |scheduler|
      scheduler.new
    end
  end
end

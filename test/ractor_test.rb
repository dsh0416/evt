# frozen_string_literal: true
require "test_helper"

class RactorTest < Minitest::Test
  def test_scheduler_ractor_compatible
    # assert not raise
    Ractor.new do
      Evt::Scheduler.new
    end
  end

  # TODO: Enrich Ractor tests after GitHub #3971 been merged
end

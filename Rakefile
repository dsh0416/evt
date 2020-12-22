require "bundler/gem_tasks"
require "rake/testtask"
require 'rake/extensiontask'

spec = Gem::Specification.load('evt.gemspec')
Rake::ExtensionTask.new('evt_ext', spec) do |ext|
  ext.ext_dir = "ext/evt"
end

Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.libs << "lib"
  t.test_files = FileList["test/**/*_test.rb"]
  t.verbose = true
end

task :default => :test

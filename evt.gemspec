require_relative 'lib/evt/version'

Gem::Specification.new do |spec|
  spec.name          = "evt"
  spec.version       = Evt::VERSION
  spec.authors       = ["Delton Ding"]
  spec.email         = ["dsh0416@gmail.com"]

  spec.summary       = "A low-level Event Handler designed for Ruby 3 Scheduler"
  spec.description   = "A low-level Event Handler designed for Ruby 3 Scheduler for better performance"
  spec.homepage      = "https://github.com/dsh0416/evt"
  spec.license       = 'BSD-3-Clause'
  spec.required_ruby_version = '>= 2.8.0.dev'

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = "https://github.com/dsh0416/evt"

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  spec.files         = Dir.chdir(File.expand_path('..', __FILE__)) do
    `git ls-files -z`.split("\x0").reject { |f| f.match(%r{^(test|spec|features)/}) }
  end
  spec.require_paths = ["lib"]
  spec.extensions = ['ext/evt/extconf.rb']

  spec.add_development_dependency 'rake-compiler', '~> 1.0'
end

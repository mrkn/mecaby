# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'mecaby/version'

Gem::Specification.new do |spec|
  spec.name          = "mecaby"
  spec.version       = Mecaby::VERSION
  spec.authors       = ["Kenta Murata"]
  spec.email         = ["mrkn@cookpad.com"]
  spec.summary       = %q{Mecaby: MeCab wrapper library for Ruby.}
  spec.description   = %q{Mecaby is an Ruby extension library to use MeCab.}
  spec.homepage      = "https://github.com/mrkn/mecaby"
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.5"
  spec.add_development_dependency "rake"
end

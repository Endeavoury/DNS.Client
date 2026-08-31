Gem::Specification.new do |spec|
  spec.name = "ratatoskr-sdk"
  spec.version = "0.1.0"
  spec.authors = ["Ratatoskr contributors"]
  spec.summary = "Ruby bindings for the Ratatoskr native networking core"
  spec.description = "Ownership-safe Ruby Fiddle bindings to the canonical Ratatoskr C ABI."
  spec.homepage = "https://github.com/Endeavoury/Ratatoskr"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.1"
  spec.files = Dir["lib/**/*.rb", "README.md"]
  spec.require_paths = ["lib"]
  spec.metadata = { "source_code_uri" => spec.homepage, "rubygems_mfa_required" => "true" }
end

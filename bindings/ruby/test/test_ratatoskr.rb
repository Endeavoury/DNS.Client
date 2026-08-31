require "minitest/autorun"
require "ratatoskr"

class RatatoskrTest < Minitest::Test
  def test_native_abi = assert_equal(1, Ratatoskr.abi_version)
end

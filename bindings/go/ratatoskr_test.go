package ratatoskr

import "testing"

func TestNativeABI(t *testing.T) {
	if ABI() != 1 {
		t.Fatalf("ABI = %d", ABI())
	}
}

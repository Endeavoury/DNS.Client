package io.github.endeavoury.ratatoskr.kotlin

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test

class RatatoskrTest { @Test fun loadsNativeAbi() = assertEquals(1, Ratatoskr.abiVersion) }

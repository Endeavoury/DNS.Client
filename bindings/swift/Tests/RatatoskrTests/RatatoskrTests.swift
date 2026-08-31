import XCTest
@testable import Ratatoskr

final class RatatoskrTests: XCTestCase { func testNativeABI() { XCTAssertEqual(Dns.abiVersion, 1) } }

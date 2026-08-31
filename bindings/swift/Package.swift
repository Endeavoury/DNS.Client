// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "Ratatoskr",
    platforms: [.macOS(.v11), .iOS(.v13)],
    products: [.library(name: "Ratatoskr", targets: ["Ratatoskr"])],
    targets: [
        .systemLibrary(name: "CRatatoskr", pkgConfig: "ratatoskr", providers: [.brew(["ratatoskr"]), .apt(["libratatoskr-dev"])]),
        .target(name: "Ratatoskr", dependencies: ["CRatatoskr"]),
        .testTarget(name: "RatatoskrTests", dependencies: ["Ratatoskr"])
    ]
)

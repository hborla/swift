// REQUIRES: swift_swift_parser, executable_test

// RUN: %target-swift-frontend -swift-version 5 -typecheck -parse-as-library -enable-experimental-feature InitAccessors -plugin-path %swift-host-lib-dir/plugins -primary-file %s %S/Inputs/Other.swift

@main
struct Main {
  static func main() {
    print("Success")
  }
}

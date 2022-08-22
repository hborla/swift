// RUN: %target-typecheck-verify-swift -enable-experimental-feature RegistryTypes

@registry
struct RegistryStruct {}

@registry
class RegistryClass {}

@registry
actor RegistryActor {}

@registry // expected-error {{'@registry' attribute cannot be applied to this declaration}}
enum RegistryEnum {}

@registry // expected-error {{'@registry' attribute cannot be applied to this declaration}}
protocol RegistryProtocol {}

@registry // expected-error {{'@registry' attribute cannot be applied to this declaration}}
extension Int {}

@registry // expected-error {{'@registry' attribute cannot be applied to this declaration}}
var registryVar: Int = 10

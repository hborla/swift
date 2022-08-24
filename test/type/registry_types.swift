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

@registry
struct ConformanceCollector {}

@ConformanceCollector
protocol P {}

@ConformanceCollector
struct ConformingStruct: P {}

@ConformanceCollector
struct GenericStruct<T>: P {}
  // expected-error@-2 {{Cannot register generic struct 'GenericStruct' with registry 'ConformanceCollector'}}

@ConformanceCollector
extension Int: P {}
// expected-error@-2 {{Registry type 'ConformanceCollector' can only be applied to non-generic types and top-level functions}}

@registry
struct FunctionCollector {}

@FunctionCollector
func global() {}

@FunctionCollector
func globalGeneric<T>(value: T) {}
// expected-error@-2 {{Cannot register global function 'globalGeneric' with registry 'FunctionCollector'}}

struct TypeContext {
  @FunctionCollector
  func instanceMethod() {}
  // expected-error@-2 {{Cannot register instance method 'instanceMethod' with registry 'FunctionCollector'}}

  @FunctionCollector
  static func staticMethod() {}

  @FunctionCollector
  let property: Int
  // expected-error@-2 {{Cannot register property 'property' with registry 'FunctionCollector'}}
}

@ConformanceCollector
@FunctionCollector
struct MultipleRegistrations {}
// expected-error@-1 {{struct 'MultipleRegistrations' cannot have multiple registry attributes}}


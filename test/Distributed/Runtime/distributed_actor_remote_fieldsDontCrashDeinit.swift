// RUN: %empty-directory(%t)
// RUN: %target-swift-frontend-emit-module -emit-module-path %t/FakeDistributedActorSystems.swiftmodule -module-name FakeDistributedActorSystems -disable-availability-checking %S/../Inputs/FakeDistributedActorSystems.swift
// RUN: %target-build-swift -module-name main -Xfrontend -enable-experimental-distributed -Xfrontend -disable-availability-checking -j2 -parse-as-library -I %t %s %S/../Inputs/FakeDistributedActorSystems.swift -o %t/a.out
// RUN: %target-run %t/a.out | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: concurrency
// REQUIRES: distributed

// rdar://76038845
// UNSUPPORTED: use_os_stdlib
// UNSUPPORTED: back_deployment_runtime

import _Distributed

distributed actor SomeSpecificDistributedActor {
  typealias Transport = FakeActorSystem

  let name: String
  let surname: String
  let age: Int

  init(name: String, system: FakeActorSystem) {
    self.name = name
    self.surname = "Surname"
    self.age = 42
  }

  deinit {
    print("deinit \(self.id)")
  }

  distributed func hello() async throws -> String {
    "Hello, from \(name)"
  }
}

typealias DefaultDistributedActorSystem = FakeActorSystem

// ==== Execute ----------------------------------------------------------------

func test_remote() async {
  let address = ActorAddress(parse: "sact://127.0.0.1/example#1234")
  let system = DefaultDistributedActorSystem()

  var remote: SomeSpecificDistributedActor? =
      try! SomeSpecificDistributedActor.resolve(id: address, using: system)
  // Check the id and system are the right values, and not trash memory
  print("remote.id = \(remote!.id)") // CHECK: remote.id = ActorAddress(address: "sact://127.0.0.1/example#1234")
  print("remote.system = \(remote!.actorSystem)")

  remote = nil // CHECK: deinit ActorAddress(address: "sact://127.0.0.1/example#1234")
  print("done")
}

@main struct Main {
  static func main() async {
    await test_remote()
  }
}


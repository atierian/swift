//===------------ BinaryScanningTool.cpp - Swift Compiler ----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2021 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/StaticMirror/BinaryScanningTool.h"
#include "swift/Basic/Unreachable.h"
#include "swift/Demangling/Demangler.h"
#include "swift/Reflection/ReflectionContext.h"
#include "swift/Reflection/TypeRefBuilder.h"
#include "swift/Reflection/TypeLowering.h"
#include "swift/Remote/CMemoryReader.h"
#include "swift/StaticMirror/ObjectFileContext.h"

#include "llvm/ADT/StringSet.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/MachOUniversal.h"

#include <sstream>

using namespace llvm::object;
using namespace swift::reflection;

namespace swift {
namespace static_mirror {

BinaryScanningTool::BinaryScanningTool(
    const std::vector<std::string> &binaryPaths, const std::string Arch) {
  for (const std::string &BinaryFilename : binaryPaths) {
    auto BinaryOwner = unwrap(createBinary(BinaryFilename));
    Binary *BinaryFile = BinaryOwner.getBinary();

    // The object file we are doing lookups in -- either the binary itself, or
    // a particular slice of a universal binary.
    std::unique_ptr<ObjectFile> ObjectOwner;
    const ObjectFile *O = dyn_cast<ObjectFile>(BinaryFile);
    if (!O) {
      auto Universal = cast<MachOUniversalBinary>(BinaryFile);
      ObjectOwner = unwrap(Universal->getMachOObjectForArch(Arch));
      O = ObjectOwner.get();
    }

    // Retain the objects that own section memory
    BinaryOwners.push_back(std::move(BinaryOwner));
    ObjectOwners.push_back(std::move(ObjectOwner));
    ObjectFiles.push_back(O);
  }
  Context = makeReflectionContextForObjectFiles(ObjectFiles);
  PointerSize = Context->PointerSize;
}

ConformanceCollectionResult
BinaryScanningTool::collectConformances(const std::vector<std::string> &protocolNames) {
  switch (PointerSize) {
    case 4:
      // FIXME: This could/should be configurable.
#if SWIFT_OBJC_INTEROP
      return Context->Builder.collectAllConformances<WithObjCInterop, 4>();
#else
      return Context->Builder.collectAllConformances<NoObjCInterop, 4>();
#endif
    case 8:
#if SWIFT_OBJC_INTEROP
      return Context->Builder.collectAllConformances<WithObjCInterop, 8>();
#else
      return Context->Builder.collectAllConformances<NoObjCInterop, 8>();
#endif
    default:
      fputs("unsupported word size in object file\n", stderr);
      abort();
  }
}

AssociatedTypeCollectionResult
BinaryScanningTool::collectAssociatedTypes(const std::string &mangledTypeName) {
  return Context->Builder.collectAssociatedTypes(mangledTypeName);
}

AssociatedTypeCollectionResult
BinaryScanningTool::collectAllAssociatedTypes() {
  return Context->Builder.collectAssociatedTypes(llvm::Optional<std::string>());
}
} // end namespace static_mirror
} // end namespace swift

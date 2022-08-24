//===--- TypeCheckRegistryType.cpp - registry type checking ---------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "TypeChecker.h"
#include "swift/AST/ASTContext.h"
#include "swift/AST/Decl.h"
#include "swift/AST/NameLookupRequests.h"
#include "swift/AST/Type.h"
#include "swift/AST/TypeCheckRequests.h"

using namespace swift;

CustomAttr *
AttachedRegistryAttr::evaluate(Evaluator &evaluator,
                               ValueDecl *value) const {
  DeclContext *dc = value->getDeclContext();

  // Collect registry attributes directly attached to this decl.
  llvm::TinyPtrVector<CustomAttr *> result;
  for (auto attr : value->getAttrs().getAttributes<CustomAttr>()) {
    auto mutableAttr = const_cast<CustomAttr *>(attr);
    auto nominal = evaluateOrDefault(
        evaluator,
        CustomAttrNominalRequest{mutableAttr, dc},
        nullptr);

    if (!nominal || !nominal->getAttrs().hasAttribute<RegistryAttr>())
      continue;

    // If the nominal type is a registry type, verify that 'value' is:
    //   - A protocol,
    //   - A non-generic nominal type, or
    //   - A non-generic top-level function

    auto diagnoseInvalidAttr = [&]() {
      diagnoseAndRemoveAttr(value, mutableAttr,
                            diag::invalid_registered_value_decl,
                            value->getDescriptiveKind(),
                            value->getName().getBaseIdentifier(),
                            nominal->getName());
    };

    if (!dyn_cast<NominalTypeDecl>(value) && !dyn_cast<FuncDecl>(value)) {
      diagnoseInvalidAttr();
      continue;
    }

    if (!isa<ProtocolDecl>(value) &&
        value->getInnermostDeclContext()->isGenericContext()) {
      diagnoseInvalidAttr();
      continue;
    }

    if (isa<FuncDecl>(value) &&
        !value->getDeclContext()->isModuleScopeContext() &&
        !value->isStatic()) {
      diagnoseInvalidAttr();
      continue;
    }

    result.push_back(mutableAttr);
  }

  if (result.empty())
    return nullptr;

  if (result.size() > 1) {
    value->diagnose(diag::multiple_registry_types,
                    value->getDescriptiveKind(),
                    value->getName().getBaseIdentifier());
    return nullptr;
  }

  return result.front();
}

Type
AttachedRegistryType::evaluate(Evaluator &evaluator,
                               ValueDecl *value) const {
  DeclContext *dc = value->getDeclContext();

  auto *registryTypeAttr = value->getAttachedRegistryAttr();
  if (!registryTypeAttr)
    return Type();

  Type registryType = evaluateOrDefault(
      evaluator,
      CustomAttrTypeRequest{registryTypeAttr, dc,
                            CustomAttrTypeKind::NonGeneric},
      Type());

  if (!registryType || registryType->hasError())
    return Type();

  return registryType;
}

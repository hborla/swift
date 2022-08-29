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

Expr *
RegistryTypeRecord::evaluate(Evaluator &evaluator,
                             ValueDecl *value) const {
  ASTContext &ctx = value->getASTContext();
  DeclContext *dc = value->getDeclContext();

  auto *registryAttr = value->getAttachedRegistryAttr();
  auto registryType = value->getAttachedRegistryType();
  if (!registryAttr || !registryType)
    return nullptr;

  Expr *initArgument = nullptr;
  if (auto *nominal = dyn_cast<NominalTypeDecl>(value)) {
    // Registry attributes on protocols are only used for
    // inference on conforming types.
    if (isa<ProtocolDecl>(nominal))
      return nullptr;

    // Form an initializer call passing in the metatype
    auto *metatype =
        TypeExpr::createImplicit(nominal->getDeclaredType(), ctx);
    initArgument = new (ctx) DotSelfExpr(metatype, SourceLoc(), SourceLoc());
  } else if (auto *func = dyn_cast<FuncDecl>(value)) {
    // Form an initializer call passing in the function reference
    if (func->isStatic()) {
      auto *decl = func->getDeclContext()->getAsDecl();
      auto *nominal = dyn_cast<NominalTypeDecl>(decl);
      auto *metatype =
          TypeExpr::createImplicit(nominal->getDeclaredType(), ctx);
      initArgument =
          UnresolvedDotExpr::createImplicit(ctx, metatype, func->getName());
    } else {
      initArgument = new (ctx) DeclRefExpr(
          ConcreteDeclRef(func), DeclNameLoc(), /*implicit=*/true);
    }
  }

  auto reprRange = SourceRange();
  if (auto *repr = registryAttr->getTypeRepr()) {
    reprRange = repr->getSourceRange();
  }

  auto typeExpr = TypeExpr::createImplicitHack(reprRange.Start,
                                               registryType,
                                               ctx);

  // Add the initializer argument at the front of the argument list
  SmallVector<Argument, 4> newArgs;
  newArgs.push_back(Argument::unlabeled(initArgument));
  if (auto *attrArgs = registryAttr->getArgs())
    newArgs.append(attrArgs->begin(), attrArgs->end());

  ArgumentList *argList = ArgumentList::createImplicit(
      ctx, reprRange.Start, newArgs, reprRange.End);
  Expr *init = CallExpr::createImplicit(ctx, typeExpr, argList);

  TypeChecker::typeCheckExpression(init, dc);

  return init;
}

import SwiftDiagnostics
import SwiftOperators
import SwiftSyntax
import SwiftSyntaxBuilder
import _SwiftSyntaxMacros

public struct WrapAllProperties: MemberAttributeMacro {
  public static func expansion(
    of node: AttributeSyntax,
    attachedTo decl: DeclSyntax,
    annotating member: DeclSyntax,
    in context: inout MacroExpansionContext
  ) throws -> [AttributeSyntax] {
    guard member.is(VariableDeclSyntax.self) else {
      return []
    }

    return [
      AttributeSyntax(
        attributeName: SimpleTypeIdentifierSyntax(
          name: .identifier("Wrapper")
        )
      )
      .withLeadingTrivia([.newlines(1), .spaces(2)])
    ]
  }
}


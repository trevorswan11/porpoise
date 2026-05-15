#include "sema/symbol.hh"

#include <ranges>
#include <string_view>
#include <variant>

#include <fmt/format.h>

#include "ast/kind.hh"
#include "ast/statement.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::sema {

namespace {

[[nodiscard]] auto symbol_location_of(const mod::Module&         module,
                                      const SymbolicNodeVariant& payload) noexcept
    -> SourceLocation {
    return std::visit(
        Overloaded{
            [&](const SymbolicNode& node) { return module.ast.location_of(*node); },
            [&](const VirtualSymbol&) { return SourceLocation{0, 0}; },
            [&](const SymbolicUnionField& inner) { return module.ast.location_of(*inner.ident); },
            [&](const SymbolicEnumeration& inner) { return module.ast.location_of(*inner.first); },
            [&](const SymbolicSelfParam& inner) { return module.ast.location_of(*inner.ident); },
            [&](const SymbolicParam& inner) {
                if (inner.ident) { return module.ast.location_of(**inner.ident); }
                return module.ast.location_of(inner.explicit_type);
            },
            [&](const SymbolicCapture& inner) { return module.ast.location_of(*inner.payload); },
            [&](const SymbolicArm& inner) { return module.ast.location_of(*inner.pattern); },
            [&](const SymbolicImport& inner) { return module.ast.location_of(*inner.node); }},
        payload);
}

} // namespace

auto Symbol::get_symbol_location(const mod::Module& module) const noexcept -> SourceLocation {
    return symbol_location_of(module, node_);
}

auto Symbol::is_public(const mod::Module& module) const noexcept -> bool {
    return match(
        Overloaded{[&](const SymbolicNode& node) {
                       switch (node->get_kind()) {
                       case ast::NodeKind::DECL_STATEMENT:
                           return module.ast.get_as<ast::DeclStatement>(*node).has_modifier(
                               ast::DeclModifiers::PUBLIC);
                       case ast::NodeKind::USING_STATEMENT:
                           return node->get_token_type() == syntax::TokenType::PUBLIC;
                       default: return false;
                       }
                   },
                   [](const SymbolicImport& import_stmt) {
                       return import_stmt.node->get_token_type() == syntax::TokenType::PUBLIC;
                   },
                   [](const auto&) { return false; }});
}

auto SymbolTable::insert(std::string_view name, const mod::Module& module, SymbolicNodeVariant node)
    -> Result<Unit, Diagnostic> {
    // Reserved identifier use is impossible due to a parser invariant
    auto [it, inserted] = symbols_.try_emplace(name, name, node);

    // Check for redeclaration since there's no shadowing
    if (!inserted) {
        return make_sema_err(
            fmt::format("Redeclaration of symbol '{}'. Previous declaration here: {}",
                        name,
                        it->second.get_symbol_location(module)),
            Error::IDENTIFIER_REDECLARATION,
            symbol_location_of(module, node));
    }
    return Unit{};
}

auto SymbolTable::insert_unchecked(std::string_view name, SymbolicNodeVariant node) -> void {
    // Reserved identifier use is impossible due to a parser invariant
    auto [_, inserted] = symbols_.try_emplace(name, name, node);
    ASSERT(inserted, "Duplicate symbol injected");
}

auto SymbolTableRegistry::insert_into(usize               table_idx,
                                      const mod::Module&  module,
                                      std::string_view    name,
                                      SymbolicNodeVariant node) -> Result<Unit, Diagnostic> {
    if (auto table = get_opt(table_idx)) { return table->insert(name, module, node); }
    return make_sema_err(Error::INVALID_TABLE_IDX);
}

[[nodiscard]] auto SymbolTableRegistry::is_shadowing(const SymbolTableStack& stack,
                                                     const mod::Module&      module,
                                                     std::string_view        name,
                                                     SymbolicNodeVariant     node) noexcept
    -> Result<Unit, Diagnostic> {
    for (const auto idx : stack | std::views::take(stack.size() - 1)) {
        if (const auto symbol = get(idx).get_opt(name)) {
            return make_sema_err(
                fmt::format("Attempt to shadow identifier '{}'. Previous declaration here: {}",
                            name,
                            symbol_location_of(module, symbol->get_node())),
                Error::SHADOWING_DECLARATION,
                symbol_location_of(module, node));
        }
    }
    return Unit{};
}

} // namespace porpoise::sema

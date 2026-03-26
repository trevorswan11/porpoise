#include <ranges>
#include <type_traits>

#include <fmt/format.h>

#include "sema/symbol.hpp"

#include "ast/expressions/enum.hpp"       // IWYU pragma: keep
#include "ast/expressions/identifier.hpp" // IWYU pragma: keep
#include "ast/expressions/union.hpp"      // IWYU pragma: keep
#include "ast/statements/declaration.hpp" // IWYU pragma: keep
#include "ast/statements/import.hpp"      // IWYU pragma: keep
#include "ast/statements/using.hpp"       // IWYU pragma: keep

namespace porpoise::sema {

auto Symbol::is_equal(const Symbol& other) const noexcept -> bool {
    const auto names_eq = name_ == other.name_;
    const auto nodes_eq =
        node_.index() == other.node_.index() &&
        std::visit(
            [&other](const auto& v) {
                return *v == *std::get<std::remove_cvref_t<decltype(v)>>(other.node_);
            },
            node_);
    const auto types_eq = optional::safe_eq<Type&>(
        type_, other.type_, [](const Type& a, const Type& b) { return &a == &b; });
    return names_eq && nodes_eq && types_eq;
}

auto SymbolTable::insert(std::string_view name, SymbolicNode node)
    -> Expected<std::monostate, Diagnostic> {
    // Reserved identifier use is impossible due to a parser invariant
    auto [it, inserted] = symbols_.try_emplace(name, name, node);

    // Check for redeclaration since there's no shadowing
    if (!inserted) {
        return make_sema_unexpected(
            fmt::format("Redeclaration of symbol '{}'. Previous declaration here: {}",
                        name,
                        it->second.match([](const auto& inner) {
                            return SourceInfo<syntax::Token>::get(inner->get_token());
                        })),
            Error::IDENTIFIER_REDECLARATION,
            std::visit([](const auto& inner) { return inner->get_token(); }, node));
    }
    return std::monostate{};
}

auto SymbolTableRegistry::insert_into(usize table_idx, std::string_view name, SymbolicNode node)
    -> Expected<std::monostate, Diagnostic> {
    if (auto table = get_opt(table_idx)) { return table->insert(name, node); }
    return make_sema_unexpected(Error::INVALID_TABLE_IDX);
}

[[nodiscard]] auto SymbolTableRegistry::is_shadowing(const SymbolTableStack& stack,
                                                     std::string_view        name,
                                                     SymbolicNode            node) noexcept
    -> Expected<std::monostate, Diagnostic> {
    for (const auto idx : stack | std::views::take(stack.size() - 1)) {
        if (const auto symbol = get(idx).get_opt(name)) {
            return make_sema_unexpected(
                fmt::format("Attempt to shadow identifier '{}'. Previous declaration here: {}",
                            name,
                            symbol->match([](const auto& inner) {
                                return SourceInfo<syntax::Token>::get(inner->get_token());
                            })),
                Error::SHADOWING_DECLARATION,
                std::visit([](const auto& inner) { return inner->get_token(); }, node));
        }
    }
    return std::monostate{};
}

} // namespace porpoise::sema

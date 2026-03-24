#include <fmt/format.h>
#include <type_traits>

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
    const auto types_eq = true; // TODO
    return names_eq && nodes_eq && types_eq;
}

auto SymbolTable::insert(std::string_view name, SymbolicNode node)
    -> Expected<std::monostate, SemaDiagnostic> {
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
            SemaError::IDENTIFIER_REDECLARATION,
            std::visit([](const auto& inner) { return inner->get_token(); }, node));
    }
    return std::monostate{};
}

auto SymbolTable::has(std::string_view name) const noexcept -> bool {
    return symbols_.contains(name);
}

auto SymbolTableRegistry::create() -> std::pair<SymbolTable&, usize> {
    auto& table = tables_.emplace_back();
    return {table, tables_.size() - 1};
}

auto SymbolTableRegistry::get_opt(usize idx) noexcept -> Optional<SymbolTable&> {
    if (idx >= tables_.size()) { return std::nullopt; }
    return tables_[idx];
}

auto SymbolTableRegistry::get(usize idx) -> SymbolTable& { return tables_.at(idx); }

} // namespace porpoise::sema

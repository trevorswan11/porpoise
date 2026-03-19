#include <fmt/format.h>

#include "sema/symbol.hpp"

#include "ast/statements/declaration.hpp" // IWYU pragma: keep
#include "ast/statements/import.hpp"      // IWYU pragma: keep
#include "ast/statements/using.hpp"       // IWYU pragma: keep

namespace porpoise::sema {

auto SymbolTable::insert(std::string_view name, SymbolicNode node)
    -> Expected<std::monostate, SemaDiagnostic> {
    // Reserved identifier use is impossible due to a parser invariant
    auto [_, inserted] = symbols_.try_emplace(name, name, node);

    // Check for redeclaration since there's no shadowing
    if (!inserted) {
        return make_sema_unexpected(
            fmt::format("Redeclaration of symbol '{}'", name),
            SemaError::IDENTIFIER_REDECLARATION,
            std::visit([](const auto* inner) { return inner->get_token(); }, node));
    }
    return std::monostate{};
}

auto SymbolTable::get(std::string_view name) noexcept -> Optional<Symbol&> {
    auto it = symbols_.find(name);
    if (it == symbols_.end()) { return nullopt; }
    return it->second;
}

} // namespace porpoise::sema

#pragma once

#include <cassert>
#include <string_view>

#include <ankerl/unordered_dense.h>

#include "sema/error.hpp"

#include "common.hpp"
#include "expected.hpp"
#include "optional.hpp"
#include "variant.hpp"

namespace porpoise {

namespace ast {

class Node;
class UsingStatement;
class DeclStatement;
class ImportStatement;

} // namespace ast

namespace sema {

using SymbolicDecl   = NonNull<const ast::DeclStatement>;
using SymbolicImport = NonNull<const ast::ImportStatement>;
using SymbolicUsing  = NonNull<const ast::UsingStatement>;

// No other nodes can ever be at the top level
using SymbolicNode = std::variant<SymbolicDecl, SymbolicImport, SymbolicUsing>;

class Type;

class Symbol {
  public:
    explicit Symbol(std::string_view name, SymbolicNode node) noexcept : name_{name}, node_{node} {}

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(node, const SymbolicNode&)

    MAKE_VARIANT_UNPACKER(decl_stmt, ast::DeclStatement, SymbolicDecl, node_, *std::get)
    MAKE_VARIANT_UNPACKER(using_stmt, ast::ImportStatement, SymbolicImport, node_, *std::get)
    MAKE_VARIANT_UNPACKER(import_stmt, ast::UsingStatement, SymbolicUsing, node_, *std::get)

    MAKE_VARIANT_MATCHER(node_)

    // Fills the internal type. Should only be called once per Symbol.
    auto resolve(Type& type) const noexcept -> void { // cppcheck-suppress constParameterReference
        assert(!type_);
        type_ = type;
    }

    [[nodiscard]] auto has_type() const noexcept -> bool { return type_.has_value(); }
    [[nodiscard]] auto get_type() const noexcept -> Type& { return *type_; }

    MAKE_EQ_DELEGATION(Symbol)

  private:
    std::string_view              name_;
    SymbolicNode                  node_;
    mutable Optional<sema::Type&> type_; // Not populated until pass 2
};

class SymbolTable {
  public:
    using Table = ankerl::unordered_dense::map<std::string_view, Symbol>;
    using KV    = Table::iterator::value_type;

    using iterator       = typename Table::iterator;
    using const_iterator = typename Table::const_iterator;

  public:
    SymbolTable() noexcept = default;

    [[nodiscard]] auto begin() noexcept -> iterator { return symbols_.begin(); }
    [[nodiscard]] auto end() noexcept -> iterator { return symbols_.end(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return symbols_.begin(); }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return symbols_.end(); }

    auto insert(std::string_view name, SymbolicNode node)
        -> Expected<std::monostate, SemaDiagnostic>;

    auto reserve(usize cap) -> void { symbols_.reserve(cap); }

    [[nodiscard]] auto empty() const noexcept -> bool { return symbols_.empty(); }
    [[nodiscard]] auto size() const noexcept -> usize { return symbols_.size(); }
    [[nodiscard]] auto has(std::string_view name) const noexcept -> bool;

    // cppcheck-suppress-begin functionStatic

    // Differs from `get_opt` by asserting that the name is present.
    template <typename Self>
    [[nodiscard]] auto get(this Self&& self, std::string_view name) noexcept -> auto& {
        auto it = self.symbols_.find(name);
        assert(it != self.symbols_.end() && "Illegal get on missing key");
        return it->second;
    }

    // Returns an optional containing a mutable or const reference to a symbol depending on context.
    template <typename Self>
    [[nodiscard]] auto get_opt(this Self&& self, std::string_view name) noexcept
        -> Optional<std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                       const Symbol&,
                                       Symbol&>> {
        auto it = self.symbols_.find(name);
        if (it == self.symbols_.end()) { return std::nullopt; }
        return it->second;
    }

    // cppcheck-suppress-end functionStatic

    // Treat this symbol table as an importable module in future passes
    auto indicate_module() noexcept -> void { is_module_ = true; }
    auto is_module() const noexcept -> bool { return is_module_; }

  private:
    Table symbols_;
    bool  is_module_{false};
};

} // namespace sema

} // namespace porpoise

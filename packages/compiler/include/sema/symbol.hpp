#pragma once

#include <cassert>
#include <string_view>
#include <vector>

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

class UnionField;
class Enumeration;

} // namespace ast

namespace sema {

using SymbolicDecl        = NonNull<const ast::DeclStatement>;
using SymbolicImport      = NonNull<const ast::ImportStatement>;
using SymbolicUsing       = NonNull<const ast::UsingStatement>;
using SymbolicUnionField  = NonNull<const ast::UnionField>;
using SymbolicEnumeration = NonNull<const ast::Enumeration>;

// No other nodes can ever be at the top level
using SymbolicNode = std::
    variant<SymbolicDecl, SymbolicImport, SymbolicUsing, SymbolicUnionField, SymbolicEnumeration>;

class Type;

class Symbol {
  public:
    explicit Symbol(std::string_view name, SymbolicNode node) noexcept : name_{name}, node_{node} {}

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(node, const SymbolicNode&)

    MAKE_VARIANT_UNPACKER(decl_stmt, ast::DeclStatement, SymbolicDecl, node_, *std::get)
    MAKE_VARIANT_UNPACKER(import_stmt, ast::ImportStatement, SymbolicImport, node_, *std::get)
    MAKE_VARIANT_UNPACKER(using_stmt, ast::UsingStatement, SymbolicUsing, node_, *std::get)
    MAKE_VARIANT_UNPACKER(union_field, ast::UnionField, SymbolicUnionField, node_, *std::get)
    MAKE_VARIANT_UNPACKER(enumeration, ast::Enumeration, SymbolicEnumeration, node_, *std::get)

    MAKE_VARIANT_MATCHER(node_)

    auto               emplace_type(Type& type) const noexcept -> void { type_.emplace(type); }
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
    MAKE_UNALIASED_ITERATOR(Table, symbols_)
    using KV = Table::iterator::value_type;

  public:
    auto insert(std::string_view name, SymbolicNode node)
        -> Expected<std::monostate, SemaDiagnostic>;

    auto reserve(usize cap) -> void { symbols_.reserve(cap); }

    [[nodiscard]] auto has(std::string_view name) const noexcept -> bool {
        return symbols_.contains(name);
    }

    // Differs from `get_opt` by asserting that the name is present.
    [[nodiscard]] auto get(std::string_view name) const noexcept -> const Symbol& {
        auto it = symbols_.find(name);
        assert(it != symbols_.end() && "Illegal get on missing key");
        return it->second;
    }

    // Returns an optional containing a mutable or const reference to a symbol depending on context.
    [[nodiscard]] auto get_opt(std::string_view name) const noexcept -> Optional<const Symbol&> {
        auto it = symbols_.find(name);
        if (it == symbols_.end()) { return std::nullopt; }
        return it->second;
    }

    // Treat this symbol table as an importable module in future passes
    auto indicate_module() noexcept -> void { is_module_ = true; }
    auto is_module() const noexcept -> bool { return is_module_; }

  private:
    Table symbols_;
    bool  is_module_{false};
};

class SymbolTableStack {
  public:
    class Guard {
      public:
        Guard(SymbolTableStack& s, usize idx) : stack_{s} { stack_.push(idx); }
        ~Guard() { stack_.pop(); }

      private:
        SymbolTableStack& stack_;
    };

    MAKE_ITERATOR(Stack, std::vector<usize>, stack_)

  public:
    auto push(usize idx) -> void { stack_.push_back(idx); }
    auto pop() noexcept -> usize {
        const auto back = stack_.back();
        stack_.pop_back();
        return back;
    }
    [[nodiscard]] auto peek() const noexcept -> usize { return stack_.back(); }

  private:
    Stack stack_;
};

class SymbolTableRegistry {
  public:
    [[nodiscard]] auto create() -> usize {
        tables_.emplace_back();
        return tables_.size() - 1;
    }

    [[nodiscard]] auto insert_into(usize table_idx, std::string_view name, SymbolicNode node)
        -> Expected<std::monostate, SemaDiagnostic>;

    [[nodiscard]] auto get_opt(usize idx) noexcept -> Optional<SymbolTable&> {
        if (idx >= tables_.size()) { return std::nullopt; }
        return tables_[idx];
    }

    [[nodiscard]] auto get(usize idx) -> SymbolTable& { return tables_.at(idx); }

    [[nodiscard]] auto
    is_shadowing(const SymbolTableStack& stack, std::string_view name, SymbolicNode node) noexcept
        -> Expected<std::monostate, SemaDiagnostic>;

  private:
    std::vector<SymbolTable> tables_;
};

} // namespace sema

} // namespace porpoise

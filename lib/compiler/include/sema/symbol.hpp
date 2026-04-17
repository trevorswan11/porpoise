#pragma once

#include <cassert>
#include <deque>
#include <string_view>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "sema/error.hpp"

#include "syntax/token.hpp"

#include "expected.hpp"
#include "iterator.hpp"
#include "optional.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise {

namespace ast {

class Node;
class UsingStatement;
class DeclStatement;
class ImportStatement;

class UnionField;
class Enumeration;

class SelfParameter;
class FunctionParameter;

class ForLoopCapture;
class MatchArm;

} // namespace ast

namespace sema {

using SymbolicDecl        = NonNull<const ast::DeclStatement>;
using SymbolicImport      = NonNull<const ast::ImportStatement>;
using SymbolicUsing       = NonNull<const ast::UsingStatement>;
using SymbolicUnionField  = NonNull<const ast::UnionField>;
using SymbolicEnumeration = NonNull<const ast::Enumeration>;
using SymbolicSelfParam   = NonNull<const ast::SelfParameter>;
using SymbolicParam       = NonNull<const ast::FunctionParameter>;
using SymbolicCapture     = NonNull<const ast::ForLoopCapture>;
using SymbolicArm         = NonNull<const ast::MatchArm>;

// No other nodes can ever be at the top level
using SymbolicNode = std::variant<SymbolicDecl,
                                  SymbolicImport,
                                  SymbolicUsing,
                                  SymbolicUnionField,
                                  SymbolicEnumeration,
                                  SymbolicSelfParam,
                                  SymbolicParam,
                                  SymbolicCapture,
                                  SymbolicArm>;

class Type;

enum class ResolveStatus : u8 {
    UNRESOLVED,
    RESOLVING,
    RESOLVED,
};

class Symbol {
  public:
    Symbol(std::string_view name, SymbolicNode node) noexcept : name_{name}, node_{node} {}
    ~Symbol() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Symbol)

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(node, const SymbolicNode&)

    MAKE_VARIANT_UNPACKER(decl_stmt, ast::DeclStatement, SymbolicDecl, node_, *std::get)
    MAKE_VARIANT_UNPACKER(import_stmt, ast::ImportStatement, SymbolicImport, node_, *std::get)
    MAKE_VARIANT_UNPACKER(using_stmt, ast::UsingStatement, SymbolicUsing, node_, *std::get)
    MAKE_VARIANT_UNPACKER(union_field, ast::UnionField, SymbolicUnionField, node_, *std::get)
    MAKE_VARIANT_UNPACKER(enumeration, ast::Enumeration, SymbolicEnumeration, node_, *std::get)
    MAKE_VARIANT_UNPACKER(self_param, ast::SelfParameter, SymbolicSelfParam, node_, *std::get)
    MAKE_VARIANT_UNPACKER(basic_param, ast::FunctionParameter, SymbolicParam, node_, *std::get)
    MAKE_VARIANT_UNPACKER(for_loop_capture, ast::ForLoopCapture, SymbolicCapture, node_, *std::get)
    MAKE_VARIANT_UNPACKER(match_arm, ast::MatchArm, SymbolicArm, node_, *std::get)

    MAKE_VARIANT_MATCHER(node_)
    [[nodiscard]] auto get_node_token() const noexcept -> syntax::Token;

    auto               mark_public() noexcept -> void { public_ = true; }
    [[nodiscard]] auto is_public() const noexcept -> bool { return public_; }

    auto               emplace_type(Type& type) noexcept -> void { type_.emplace(type); }
    [[nodiscard]] auto has_type() const noexcept -> bool { return type_.has_value(); }
    [[nodiscard]] auto get_type() const noexcept -> Type& { return *type_; }

    [[nodiscard]] auto get_status() const noexcept -> ResolveStatus { return status_; }
    auto               set_status(ResolveStatus status) noexcept -> void { status_ = status; }

    MAKE_EQ_DELEGATION(Symbol)

  private:
    std::string_view      name_;
    bool                  public_{false};
    SymbolicNode          node_;
    Optional<sema::Type&> type_;
    ResolveStatus         status_{ResolveStatus::UNRESOLVED};
};

class SymbolTable {
  public:
    using Table = ankerl::unordered_dense::map<std::string_view, Symbol>;
    MAKE_UNALIASED_ITERATOR(Table, symbols_)
    using KV = Table::iterator::value_type;

  public:
    SymbolTable() noexcept = default;
    ~SymbolTable()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTable)

    auto insert(std::string_view name, SymbolicNode node) -> Expected<Unit, Diagnostic>;

    auto reserve(usize cap) -> void { symbols_.reserve(cap); }

    [[nodiscard]] auto has(std::string_view name) const noexcept -> bool {
        return symbols_.contains(name);
    }

    // Differs from `get_opt` by asserting that the name is present.
    template <typename Self>
    [[nodiscard]] auto get(this Self&& self, std::string_view name) noexcept -> auto& {
        auto it = self.symbols_.find(name);
        assert(it != self.symbols_.end() && "Illegal get on missing key");
        return it->second;
    }

    // Returns an optional containing a mutable or const reference to a symbol depending on context.
    template <typename Self>
    [[nodiscard]] auto get_opt(this Self&& self, std::string_view name) noexcept {
        auto it          = self.symbols_.find(name);
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              Optional<const Symbol&>,
                                              Optional<Symbol&>>;
        if (it == self.symbols_.end()) { return ReturnType{std::nullopt}; }
        return ReturnType{it->second};
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
        Guard(SymbolTableStack& s, usize idx) noexcept : stack_{s} { stack_.push(idx); }
        ~Guard() { stack_.pop(); }

      private:
        SymbolTableStack& stack_;
    };

    MAKE_ITERATOR(Stack, std::vector<usize>, stack_)

  public:
    SymbolTableStack() noexcept = default;
    ~SymbolTableStack()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTableStack)

    auto push(usize idx) -> void { stack_.push_back(idx); }
    auto pop() noexcept -> void {
        if (!stack_.empty()) { stack_.pop_back(); }
    }

  private:
    Stack stack_;
};

class SymbolTableRegistry {
  public:
    MAKE_ITERATOR(Tables, std::deque<SymbolTable>, tables_)

  public:
    SymbolTableRegistry() noexcept = default;
    ~SymbolTableRegistry()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SymbolTableRegistry)

    [[nodiscard]] auto create() -> usize {
        tables_.emplace_back();
        return tables_.size() - 1;
    }

    [[nodiscard]] auto insert_into(usize table_idx, std::string_view name, SymbolicNode node)
        -> Expected<Unit, Diagnostic>;

    template <typename Self> [[nodiscard]] auto get(this Self&& self, usize idx) -> auto& {
        return self.tables_.at(idx);
    }

    template <typename Self> [[nodiscard]] auto get_opt(this Self&& self, usize idx) noexcept {
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              Optional<const SymbolTable&>,
                                              Optional<SymbolTable&>>;
        if (idx >= self.tables_.size()) { return ReturnType{std::nullopt}; }
        return ReturnType{self.tables_[idx]};
    }

    template <typename Self>
    [[nodiscard]] auto get_from(this Self&& self, usize idx, std::string_view name) -> auto& {
        return self.tables_.at(idx).get(name);
    }

    template <typename Self>
    [[nodiscard]] auto get_from_opt(this Self&& self, usize idx, std::string_view name) noexcept {
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              Optional<const Symbol&>,
                                              Optional<Symbol&>>;
        if (idx >= self.tables_.size()) { return ReturnType{std::nullopt}; }
        return self.tables_[idx].get_opt(name);
    }

    [[nodiscard]] auto is_shadowing(const SymbolTableStack& stack,
                                    std::string_view        name,
                                    SymbolicNode node) noexcept -> Expected<Unit, Diagnostic>;

  private:
    Tables tables_;
};

} // namespace sema

} // namespace porpoise

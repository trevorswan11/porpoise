#pragma once

#include <deque>
#include <ranges>
#include <string_view>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "sema/error.hh"

#include "ast/id.hh"

#include "module/module.hh"

#include "syntax/token.hh"

#include "assert.hh"
#include "iterator.hh"
#include "option.hh"
#include "result.hh"
#include "utility.hh"
#include "variant.hh"

namespace porpoise::sema {

// A non-AST-based symbol for primitives and builtins
class VirtualSymbol {
  public:
    explicit VirtualSymbol(const std::pair<std::string_view, syntax::TokenType>& tok) noexcept
        : token_{tok} {}

    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token& { return token_; }

  private:
    syntax::Token token_;
};

using SymbolicNode        = ast::Handle<ast::NodeKind::DECL_STATEMENT,
                                        ast::NodeKind::USING_STATEMENT,
                                        ast::NodeKind::LABEL_EXPRESSION>;
using SymbolicUnionField  = ast::UnionExpression::Field;
using SymbolicEnumeration = ast::EnumExpression::Enumeration;
using SymbolicSelfParam   = ast::SelfParameter;
using SymbolicParam       = ast::FunctionExpression::Parameter;
using SymbolicCapture     = ast::ForLoopExpression::Capture;
using SymbolicArm         = ast::MatchExpression::Arm;

struct SymbolicImport {
    ast::ImportHandle         node;
    opt::Option<mod::Module&> imported_mod;
};

using SymbolicNodeVariant = std::variant<VirtualSymbol,
                                         SymbolicNode,
                                         SymbolicImport,
                                         SymbolicUnionField,
                                         SymbolicEnumeration,
                                         SymbolicSelfParam,
                                         SymbolicParam,
                                         SymbolicCapture,
                                         SymbolicArm>;

class Type;

enum class SymbolKind : u8 {
    TYPE,
    VALUE,
    CALLABLE,
    MODULE,
};

enum class ResolveStatus : u8 {
    UNRESOLVED,
    RESOLVING,
    RESOLVED,
};

class Symbol {
  public:
    Symbol(std::string_view name, const SymbolicNodeVariant& node) noexcept
        : name_{name}, node_{node} {}
    ~Symbol() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Symbol)

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(node, const SymbolicNodeVariant&)

    MAKE_VARIANT_UNPACKER(builtin, VirtualSymbol, VirtualSymbol, node_, std::get)
    MAKE_VARIANT_UNPACKER(symbolic_node, SymbolicNode, SymbolicNode, node_, std::get)
    MAKE_VARIANT_UNPACKER(import_stmt, SymbolicImport, SymbolicImport, node_, std::get)
    MAKE_VARIANT_UNPACKER(
        union_field, ast::UnionExpression::Field, SymbolicUnionField, node_, std::get)
    MAKE_VARIANT_UNPACKER(
        enumeration, ast::EnumExpression::Enumeration, SymbolicEnumeration, node_, std::get)
    MAKE_VARIANT_UNPACKER(self_param, ast::SelfParameter, SymbolicSelfParam, node_, std::get)
    MAKE_VARIANT_UNPACKER(
        basic_param, ast::FunctionExpression::Parameter, SymbolicParam, node_, std::get)
    MAKE_VARIANT_UNPACKER(
        for_loop_capture, ast::ForLoopExpression::Capture, SymbolicCapture, node_, std::get)
    MAKE_VARIANT_UNPACKER(match_arm, ast::MatchExpression::Arm, SymbolicArm, node_, std::get)

    MAKE_VARIANT_MATCHER(node_)
    [[nodiscard]] auto get_symbol_location(const mod::Module& module) const noexcept
        -> SourceLocation;

    // Can only be true for decls, imports, and type aliases
    [[nodiscard]] auto is_public(const mod::Module& module) const noexcept -> bool;

    [[nodiscard]] auto has_type() const noexcept -> bool { return type_.has_value(); }
    [[nodiscard]] auto get_type() const noexcept -> Type& { return *type_; }
    auto               set_type(Type& type) noexcept -> void { type_.emplace(type); }

    [[nodiscard]] auto get_status() const noexcept -> ResolveStatus { return status_; }
    auto               set_status(ResolveStatus status) noexcept -> void { status_ = status; }

    [[nodiscard]] auto has_kind() const noexcept -> bool { return kind_.has_value(); }
    [[nodiscard]] auto get_kind() const noexcept -> SymbolKind { return *kind_; }
    auto               set_kind(SymbolKind kind) noexcept -> void { kind_ = kind; }

  private:
    std::string_view         name_;
    SymbolicNodeVariant      node_;
    opt::Option<sema::Type&> type_;
    ResolveStatus            status_{ResolveStatus::UNRESOLVED};
    opt::Option<SymbolKind>  kind_;
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

    // Constructs the symbolic node in place with the provided args
    template <typename T, typename... Args>
    auto insert(std::string_view name, const mod::Module& module, Args&&... args)
        -> Result<Unit, Diagnostic> {
        return insert(name, module, SymbolicNodeVariant{T{std::forward<Args>(args)...}});
    }

    // Checks that the module was inserted without collision
    auto insert(std::string_view name, const mod::Module& module, SymbolicNodeVariant node)
        -> Result<Unit, Diagnostic>;

    // For use of prelude injection only
    auto insert_unchecked(std::string_view name, SymbolicNodeVariant node) -> void;

    auto reserve(usize cap) -> void { symbols_.reserve(cap); }

    [[nodiscard]] auto has(std::string_view name) const noexcept -> bool {
        return symbols_.contains(name);
    }

    // Differs from `get_opt` by asserting that the name is present.
    template <typename Self>
    [[nodiscard]] auto get(this Self&& self, std::string_view name) noexcept -> auto& {
        auto it = self.symbols_.find(name);
        ASSERT(it != self.symbols_.end(), "Illegal get on missing key");
        return it->second;
    }

    // Returns an optional containing a mutable or const reference to a symbol depending on context.
    template <typename Self>
    [[nodiscard]] auto get_opt(this Self&& self, std::string_view name) noexcept {
        auto it          = self.symbols_.find(name);
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              opt::Option<const Symbol&>,
                                              opt::Option<Symbol&>>;
        if (it == self.symbols_.end()) { return ReturnType{opt::none}; }
        return ReturnType{it->second};
    }

  private:
    Table symbols_;
};

class SymbolTableStack {
  public:
    // A basic push/pop RAII guard, see `Scope`
    class Guard {
      public:
        Guard(SymbolTableStack& s, usize idx) noexcept : stack_{s} { stack_.push(idx); }
        ~Guard() { stack_.pop(); }

      private:
        SymbolTableStack& stack_;
    };

    // An extension of `Guard` that also resets the old index upon destruction
    class Scope {
      public:
        Scope(SymbolTableStack& s, usize new_idx, usize& old_idx) noexcept
            : guard_{s, new_idx}, idx_ref_{old_idx}, old_idx_{old_idx} {
            old_idx = new_idx;
        }
        ~Scope() { idx_ref_ = old_idx_; }

      private:
        SymbolTableStack::Guard guard_;
        usize&                  idx_ref_;
        usize                   old_idx_;
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

#define OPTIONAL_RETURN_TYPE(Underlying)                               \
    std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, \
                       opt::Option<const Underlying&>,                 \
                       opt::Option<Underlying&>>;

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

    // Constructs the symbolic node in place with the provided args
    template <typename T, typename... Args>
    [[nodiscard]] auto
    insert_into(usize table_idx, const mod::Module& module, std::string_view name, Args&&... args)
        -> Result<Unit, Diagnostic> {
        return insert_into(
            table_idx, module, name, SymbolicNodeVariant{T{std::forward<Args>(args)...}});
    }

    [[nodiscard]] auto insert_into(usize               table_idx,
                                   const mod::Module&  module,
                                   std::string_view    name,
                                   SymbolicNodeVariant node) -> Result<Unit, Diagnostic>;

    template <typename Self> [[nodiscard]] auto get(this Self&& self, usize idx) -> auto& {
        return self.tables_.at(idx);
    }

    template <typename Self> [[nodiscard]] auto get_opt(this Self&& self, usize idx) noexcept {
        using ReturnType = OPTIONAL_RETURN_TYPE(SymbolTable);
        if (idx >= self.tables_.size()) { return ReturnType{opt::none}; }
        return ReturnType{self.tables_[idx]};
    }

    template <typename Self>
    [[nodiscard]] auto get_from(this Self&& self, usize idx, std::string_view name) -> auto& {
        return self.tables_.at(idx).get(name);
    }

    template <typename Self>
    [[nodiscard]] auto get_from_opt(this Self&& self, usize idx, std::string_view name) noexcept {
        using ReturnType = OPTIONAL_RETURN_TYPE(Symbol);
        if (idx >= self.tables_.size()) { return ReturnType{opt::none}; }
        return self.tables_[idx].get_opt(name);
    }

    // Looks up all levels of the stack for possible illegal shadowing of the name
    [[nodiscard]] auto is_shadowing(const SymbolTableStack& stack,
                                    const mod::Module&      module,
                                    std::string_view        name,
                                    SymbolicNodeVariant node) noexcept -> Result<Unit, Diagnostic>;

    // Looks up all levels of the stack for the queried name
    template <typename Self>
    [[nodiscard]] auto
    lookup(this Self&& self, const SymbolTableStack& stack, std::string_view name) noexcept {
        using ReturnType = OPTIONAL_RETURN_TYPE(Symbol);
        for (const auto idx : std::views::reverse(stack)) {
            if (auto symbol = self.tables_[idx].get_opt(name)) { return symbol; }
        }
        return ReturnType{opt::none};
    }

  private:
    Tables tables_;
};

#undef OPTIONAL_RETURN_TYPE

} // namespace porpoise::sema

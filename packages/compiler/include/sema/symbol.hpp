#pragma once

#include <cassert>
#include <string_view>

#include <ankerl/unordered_dense.h>

#include <config.h>

#include "sema/error.hpp"

#include "common.hpp"
#include "expected.hpp"
#include "variant.hpp"

namespace porpoise {

namespace ast {

class Node;
class UsingStatement;
class DeclStatement;
class ImportStatement;

} // namespace ast

namespace sema {

// No other nodes can ever be at the top level
using SymbolicNode = std::
    variant<const ast::UsingStatement*, const ast::DeclStatement*, const ast::ImportStatement*>;

class Type;

class Symbol {
  public:
    explicit Symbol(std::string_view name, SymbolicNode node) noexcept : name_{name}, node_{node} {
#ifdef ASSERTIONS
        std::visit([](const auto* inner) { assert(inner && "Sema received null node"); }, node);
#endif
    }

    MAKE_GETTER(name, std::string_view)
    MAKE_GETTER(node, const SymbolicNode&)

    MAKE_VARIANT_UNPACKER(
        using_stmt, ast::UsingStatement, const ast::UsingStatement*, node_, *std::get)
    MAKE_VARIANT_UNPACKER(
        decl_stmt, ast::DeclStatement, const ast::DeclStatement*, node_, *std::get)
    MAKE_VARIANT_UNPACKER(
        import_stmt, ast::ImportStatement, const ast::ImportStatement*, node_, *std::get)

    // Provides std::visit-like access to the internal node
    template <typename Matcher> auto match(Matcher&& matcher) { return std::visit(matcher, node_); }

    // Fills the internal type. Should only be called once per Symbol.
    auto resolve(Type* type) noexcept -> void {
        assert(type && !resolved_type_);
        resolved_type_ = type;
    }

    // Provides mutable access to the symbol's potentially invalid type.
    auto access_type() noexcept -> Type* { return resolved_type_; }

  private:
    std::string_view name_;
    SymbolicNode     node_;
    Type*            resolved_type_{nullptr}; // Not populated until pass 2
};

class SymbolTable {
  public:
    SymbolTable() noexcept = default;

    auto insert(std::string_view name, SymbolicNode node)
        -> Expected<std::monostate, SemaDiagnostic>;

  private:
    ankerl::unordered_dense::map<std::string_view, Symbol> symbols_;
};

} // namespace sema

} // namespace porpoise

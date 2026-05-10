#pragma once

#include "ast/handle.hh"

#include "syntax/error.hh"

namespace porpoise {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

namespace type_modifiers {

constexpr ast::TypeModifier VALUE{};
constexpr ast::TypeModifier REF{ast::TypeModifier::Modifier::REF};
constexpr ast::TypeModifier MUT_REF{ast::TypeModifier::Modifier::MUT_REF};
constexpr ast::TypeModifier PTR{ast::TypeModifier::Modifier::PTR};
constexpr ast::TypeModifier MUT_PTR{ast::TypeModifier::Modifier::MUT_PTR};
constexpr ast::TypeModifier VOLATILE{ast::TypeModifier::Modifier::VOLATILE};

} // namespace type_modifiers

struct ExplicitArrayType {
    opt::Option<ExpressionHandle> dimension;
    bool                          null_terminated;
    ExplicitTypeID                inner_explicit_type;
};

struct ExplicitType {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExplicitTypeID, syntax::Diagnostic>;
};

struct TypeExpression {
    opt::Option<ExplicitTypeID> explicit_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic>;
};

} // namespace ast

} // namespace porpoise

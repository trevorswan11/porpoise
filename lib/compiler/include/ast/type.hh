#pragma once

#include "ast/handle.hh"

#include "syntax/error.hh"

namespace porpoise {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

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

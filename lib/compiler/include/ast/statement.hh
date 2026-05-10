#pragma once

#include "ast/handle.hh"

#include "syntax/error.hh"

namespace porpoise {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

struct BlockStatement {
    MAKE_ITERATOR(Statements, std::vector<StatementHandle>, statements);

    Statements statements;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct BreakStatement {
    opt::Option<IdentifierHandle> label;
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ContinueStatement {
    opt::Option<IdentifierHandle> label;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct DeclStatement {
    IdentifierHandle              ident;
    TypeHandle                    type;
    opt::Option<ExpressionHandle> value;
    DeclModifiers                 modifiers;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;

    [[nodiscard]] auto has_modifier(DeclModifiers flag) const noexcept -> bool {
        return modifiers_has(modifiers, flag);
    }

  private:
    static auto modifiers_has(DeclModifiers modifiers, DeclModifiers flag) noexcept -> bool {
        return static_cast<bool>(modifiers & flag);
    }
};

struct DeferStatement {
    StatementHandle deferred;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct DiscardStatement {
    ExpressionHandle discarded;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ExpressionStatement {
    ExpressionHandle expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser, bool require_semicolon)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ImportStatement {
    ImportPayloadHandle           payload;
    opt::Option<IdentifierHandle> alias;

    [[nodiscard]] static constexpr auto is_public(const NodeID& id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ReturnStatement {
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct TestStatement {
    opt::Option<StringHandle> description;
    BlockHandle               block;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct UsingStatement {
    IdentifierHandle alias;
    ExplicitTypeID   explicit_type;

    [[nodiscard]] static constexpr auto is_public(const NodeID& id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

} // namespace ast

} // namespace porpoise

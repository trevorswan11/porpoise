#pragma once

#include <span>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class ExplicitType;
class BlockStatement;

class FunctionParameter {
  public:
    explicit FunctionParameter(mem::Box<IdentifierExpression> ident, ExplicitType&& type) noexcept;
    ~FunctionParameter();

    MAKE_AST_COPY_MOVE(FunctionParameter)

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_GETTER(type, const ExplicitType&)

    MAKE_EQ_DELEGATION(FunctionParameter)

  private:
    mem::Box<IdentifierExpression> ident_;
    ExplicitType                   type_;
};

class SelfParameter {
  public:
    explicit SelfParameter(TypeModifier modifier, mem::Box<IdentifierExpression> name) noexcept;
    ~SelfParameter();

    MAKE_AST_COPY_MOVE(SelfParameter)

    MAKE_GETTER(modifier, const TypeModifier&)
    MAKE_GETTER(ident, const IdentifierExpression&, *)

    MAKE_EQ_DELEGATION(SelfParameter)

  private:
    TypeModifier                   modifier_;
    mem::Box<IdentifierExpression> ident_;
};

class FunctionExpression : public ExprBase<FunctionExpression> {
  public:
    static constexpr auto KIND = NodeKind::FUNCTION_EXPRESSION;

  public:
    explicit FunctionExpression(const syntax::Token&               start_token,
                                Optional<SelfParameter>            self,
                                std::vector<FunctionParameter>     parameters,
                                ExplicitType&&                     return_type,
                                Optional<mem::Box<BlockStatement>> body) noexcept;
    ~FunctionExpression() override;

    MAKE_AST_COPY_MOVE(FunctionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(self, SelfParameter, self_, *)
    MAKE_GETTER(parameters, std::span<const FunctionParameter>)
    MAKE_GETTER(return_type, const ExplicitType&)
    MAKE_OPTIONAL_UNPACKER(body, BlockStatement, body_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<SelfParameter>            self_;
    std::vector<FunctionParameter>     parameters_;
    ExplicitType                       return_type_;
    Optional<mem::Box<BlockStatement>> body_;
};

} // namespace porpoise::ast

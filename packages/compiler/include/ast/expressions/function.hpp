#pragma once

#include <span>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class ExplicitType;
class BlockStatement;

class FunctionParameter {
  public:
    explicit FunctionParameter(Box<IdentifierExpression> ident, ExplicitType&& type) noexcept;
    ~FunctionParameter();

    MAKE_AST_COPY_MOVE(FunctionParameter)

    MAKE_AST_GETTER(ident, const IdentifierExpression&, *)
    MAKE_AST_GETTER(type, const ExplicitType&, )

    MAKE_AST_DEPENDENT_EQ(FunctionParameter)

  private:
    Box<IdentifierExpression> ident_;
    ExplicitType              type_;
};

class SelfParameter {
  public:
    explicit SelfParameter(TypeModifier modifier, Box<IdentifierExpression> name) noexcept;
    ~SelfParameter();

    MAKE_AST_COPY_MOVE(SelfParameter)

    MAKE_AST_GETTER(modifier, const TypeModifier&, )
    MAKE_AST_GETTER(ident, const IdentifierExpression&, *)

    MAKE_AST_DEPENDENT_EQ(SelfParameter)

  private:
    TypeModifier              modifier_;
    Box<IdentifierExpression> ident_;
};

class FunctionExpression : public ExprBase<FunctionExpression> {
  public:
    static constexpr auto KIND = NodeKind::FUNCTION_EXPRESSION;

  public:
    explicit FunctionExpression(const Token&                   start_token,
                                Optional<SelfParameter>        self,
                                std::vector<FunctionParameter> parameters,
                                ExplicitType&&                 return_type,
                                Optional<Box<BlockStatement>>  body) noexcept;
    ~FunctionExpression() override;

    MAKE_AST_COPY_MOVE(FunctionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(self, SelfParameter, self_, *)
    MAKE_AST_GETTER(parameters, std::span<const FunctionParameter>, )
    MAKE_AST_GETTER(return_type, const ExplicitType&, )
    MAKE_OPTIONAL_UNPACKER(body, BlockStatement, body_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<SelfParameter>        self_;
    std::vector<FunctionParameter> parameters_;
    ExplicitType                   return_type_;
    Optional<Box<BlockStatement>>  body_;
};

} // namespace conch::ast

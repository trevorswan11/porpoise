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
    explicit FunctionParameter(Box<IdentifierExpression> name, ExplicitType&& type) noexcept;
    ~FunctionParameter();

    MAKE_AST_COPY_MOVE(FunctionParameter)

    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto get_type() const noexcept -> const ExplicitType& { return type_; }

    friend auto operator==(const FunctionParameter& lhs, const FunctionParameter& rhs) noexcept
        -> bool {
        return lhs.is_equal(rhs);
    }

  private:
    auto is_equal(const FunctionParameter& other) const noexcept -> bool;

  private:
    Box<IdentifierExpression> name_;
    ExplicitType              type_;

    friend class FunctionExpression;
};

class SelfParameter {
  public:
    explicit SelfParameter(TypeModifier modifier, Box<IdentifierExpression> name) noexcept;
    ~SelfParameter();

    MAKE_AST_COPY_MOVE(SelfParameter)

    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto get_modifier() const noexcept -> const TypeModifier& { return modifier_; }

  private:
    TypeModifier              modifier_;
    Box<IdentifierExpression> name_;

    friend class FunctionExpression;
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
    [[nodiscard]] auto get_parameters() const noexcept -> std::span<const FunctionParameter> {
        return parameters_;
    }

    [[nodiscard]] auto get_return_type() const noexcept -> const ExplicitType& {
        return return_type_;
    }

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

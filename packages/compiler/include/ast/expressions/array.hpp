#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    explicit ArrayExpression(const syntax::Token&         start_token,
                             Optional<Box<Expression>>    size,
                             ExplicitType&&               item_type,
                             std::vector<Box<Expression>> items) noexcept;
    ~ArrayExpression() override;

    MAKE_AST_COPY_MOVE(ArrayExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(explicit_size, Expression, size_, **)
    MAKE_GETTER(item_type, const ExplicitType&)
    MAKE_GETTER(items, std::span<const Box<Expression>>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<Box<Expression>>    size_;
    ExplicitType                 item_type_;
    std::vector<Box<Expression>> items_;
};

} // namespace porpoise::ast

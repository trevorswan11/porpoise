#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class TypeExpression;

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    explicit ArrayExpression(const Token&                 start_token,
                             Optional<Box<Expression>>    size,
                             ExplicitType&&               item_type,
                             std::vector<Box<Expression>> items) noexcept;
    ~ArrayExpression() override;

    MAKE_AST_COPY_MOVE(ArrayExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto is_inferred_size() const noexcept -> bool { return !size_.has_value(); }
    [[nodiscard]] auto get_size() const noexcept -> Optional<const Expression&> {
        return size_ ? Optional<const Expression&>{**size_} : nullopt;
    }

    [[nodiscard]] auto get_item_type() const noexcept -> const ExplicitType& { return item_type_; }
    [[nodiscard]] auto get_items() const noexcept -> std::span<const Box<Expression>> {
        return items_;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<Box<Expression>>    size_;
    ExplicitType                 item_type_;
    std::vector<Box<Expression>> items_;
};

} // namespace conch::ast

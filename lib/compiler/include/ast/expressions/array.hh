#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hh"
#include "ast/node.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    ArrayExpression(const syntax::Token&              start_token,
                    mem::NullableBox<Expression>      size,
                    bool                              null_terminated,
                    ExplicitType&&                    item_type,
                    std::vector<mem::Box<Expression>> items) noexcept;
    ~ArrayExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ArrayExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(explicit_size, Expression, size_, *)
    MAKE_GETTER(item_type, const ExplicitType&)
    MAKE_GETTER(items, std::span<const mem::Box<Expression>>)
    [[nodiscard]] auto get_size() const noexcept -> usize { return items_.size(); }
    [[nodiscard]] auto is_null_terminated() const noexcept -> bool { return null_terminated_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::NullableBox<Expression>      size_;
    bool                              null_terminated_;
    ExplicitType                      item_type_;
    std::vector<mem::Box<Expression>> items_;
};

} // namespace porpoise::ast

#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IndexExpression : public ExprBase<IndexExpression> {
  public:
    static constexpr auto KIND = NodeKind::INDEX_EXPRESSION;

  public:
    explicit IndexExpression(const syntax::Token& start_token,
                             mem::Box<Expression> array,
                             mem::Box<Expression> idx) noexcept
        : ExprBase{start_token}, array_{std::move(array)}, index_{std::move(idx)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(IndexExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> array)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(array, const Expression&, *)
    MAKE_GETTER(index, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<IndexExpression>(other);
        return *array_ == *casted.array_ && *index_ == *casted.index_;
    }

  private:
    mem::Box<Expression> array_;
    mem::Box<Expression> index_;
};

} // namespace porpoise::ast

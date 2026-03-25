#pragma once

#include <span>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class FunctionExpression;
class DeclStatement;

class StructExpression : public ExprBase<StructExpression> {
  public:
    static constexpr auto KIND = NodeKind::STRUCT_EXPRESSION;

  public:
    MAKE_ITERATOR(Members, std::vector<mem::Box<DeclStatement>>, members_)

  public:
    explicit StructExpression(const syntax::Token& start_token, Members members) noexcept;
    ~StructExpression() override;

    MAKE_AST_COPY_MOVE(StructExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(members, std::span<const mem::Box<DeclStatement>>)
    [[nodiscard]] auto is_packed() const noexcept -> bool {
        return start_token_.type == syntax::TokenType::PACKED;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<mem::Box<DeclStatement>> members_;
};

} // namespace porpoise::ast

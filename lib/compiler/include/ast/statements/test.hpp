#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class StringExpression;
class BlockStatement;

class TestStatement : public StmtBase<TestStatement> {
  public:
    static constexpr auto KIND = NodeKind::TEST_STATEMENT;

  public:
    TestStatement(const syntax::Token&                 start_token,
                  Optional<mem::Box<StringExpression>> description,
                  mem::Box<BlockStatement>             block) noexcept;
    ~TestStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TestStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(description, StringExpression, description_, **)
    MAKE_GETTER(block, const BlockStatement&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<mem::Box<StringExpression>> description_;
    mem::Box<BlockStatement>             block_;
};

} // namespace porpoise::ast

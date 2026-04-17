#include "ast/statements/test.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

TestStatement::TestStatement(const syntax::Token&               start_token,
                             mem::NullableBox<StringExpression> description,
                             mem::Box<BlockStatement>           block) noexcept
    : StmtBase{start_token}, description_{std::move(description)}, block_{std::move(block)} {}
TestStatement::~TestStatement() = default;

auto TestStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto TestStatement::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    mem::NullableBox<StringExpression> description;
    if (parser.peek_token_is(syntax::TokenType::STRING)) {
        parser.advance();
        description = mem::nullable_box_from(
            downcast<StringExpression>(TRY(StringExpression::parse(parser))));

        // Empty strings aren't supported since one should just use no description
        if (description->get_value().empty()) {
            return make_parser_unexpected(syntax::ParserError::EMPTY_TEST_DESCRIPTION,
                                          description->get_token());
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    return mem::make_box<TestStatement>(
        start_token,
        std::move(description),
        downcast<BlockStatement>(TRY(BlockStatement::parse(parser, true))));
}

auto TestStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<TestStatement>(other);
    return mem::nullable_boxes_eq(description_, casted.description_) && *block_ == *casted.block_;
}

} // namespace porpoise::ast

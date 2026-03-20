#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/primitive.hpp"

namespace porpoise::tests {

namespace helpers {

template <ast::PrimitiveNode N>
auto test_primitive(std::string_view                                               input,
                    std::string_view                                               node_token_slice,
                    Optional<syntax::TokenType>                                    expected_type,
                    std::variant<typename N::value_type, syntax::ParserDiagnostic> expected_value)
    -> void {
    using T = typename N::value_type;
    syntax::Parser p{input};
    auto [ast, errors] = p.consume();

    if (std::holds_alternative<syntax::ParserDiagnostic>(expected_value)) {
        REQUIRE(errors.size() == 1);
        const auto& actual_error = errors[0];
        REQUIRE(std::get<syntax::ParserDiagnostic>(expected_value) == actual_error);
        return;
    }

    REQUIRE(errors.empty());
    REQUIRE(ast.size() == 1);

    const auto  actual{std::move(ast[0])};
    const auto& expr_stmt = into_expression_statement(*actual);
    const N     expected{syntax::Token{*expected_type, trim_semicolons(node_token_slice)},
                     std::get<T>(expected_value)};
    REQUIRE(expected == expr_stmt.get_expression());
}

template <ast::PrimitiveNode N>
auto test_primitive(std::string_view                                               input,
                    Optional<syntax::TokenType>                                    expected_type,
                    std::variant<typename N::value_type, syntax::ParserDiagnostic> expected_value)
    -> void {
    test_primitive<N>(input, input, expected_type, expected_value);
}

} // namespace helpers

TEST_CASE("Single-line string parsing") {
    using N = ast::StringExpression;
    helpers::test_primitive<N>(
        R"("This is a string";)", syntax::TokenType::STRING, R"(This is a string)");
    helpers::test_primitive<N>(
        R"("Hello, 'World'!";)", syntax::TokenType::STRING, R"(Hello, 'World'!)");
    helpers::test_primitive<N>(R"("";)", syntax::TokenType::STRING, R"()");
}

TEST_CASE("Multi-line string parsing") {
    using N = ast::StringExpression;
    helpers::test_primitive<N>("\\\\This is a string\n;",
                               "This is a string",
                               syntax::TokenType::MULTILINE_STRING,
                               "This is a string");
    helpers::test_primitive<N>("\\\\Hello, 'World'!\n\\\\\n;",
                               "Hello, 'World'!\n\\\\",
                               syntax::TokenType::MULTILINE_STRING,
                               "Hello, 'World'!\n");
    helpers::test_primitive<N>("\\\\\n;", "", syntax::TokenType::MULTILINE_STRING, "");
}

TEST_CASE("Signed integer parsing") {
    using N = ast::SignedIntegerExpression;
    helpers::test_primitive<N>("0;", syntax::TokenType::INT_10, 0);
    helpers::test_primitive<N>("0b10011101101;", syntax::TokenType::INT_2, 0b10011101101);
    helpers::test_primitive<N>("0o1234567;", syntax::TokenType::INT_8, 342'391);
    helpers::test_primitive<N>("0xFF8a91d;", syntax::TokenType::INT_16, 0xFF8a91d);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFFFF;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Signed long integer parsing") {
    using N = ast::SignedLongIntegerExpression;
    helpers::test_primitive<N>("0l;", syntax::TokenType::LINT_10, 0LL);
    helpers::test_primitive<N>("0b10011101101l;", syntax::TokenType::LINT_2, 0b10011101101LL);
    helpers::test_primitive<N>("0o1234567l;", syntax::TokenType::LINT_8, 342'391LL);
    helpers::test_primitive<N>("0xFF8a91dl;", syntax::TokenType::LINT_16, 0xFF8a91dLL);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFFFFl;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Signed size integer parsing") {
    using N = ast::ISizeIntegerExpression;
    helpers::test_primitive<N>("0z;", syntax::TokenType::ZINT_10, 0Z);
    helpers::test_primitive<N>("0b10011101101z;", syntax::TokenType::ZINT_2, 0b10011101101Z);
    helpers::test_primitive<N>("0o1234567z;", syntax::TokenType::ZINT_8, 342'391Z);
    helpers::test_primitive<N>("0xFF8a91dz;", syntax::TokenType::ZINT_16, 0xFF8a91dZ);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFz;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned integer parsing") {
    using N = ast::UnsignedIntegerExpression;
    helpers::test_primitive<N>("0u;", syntax::TokenType::UINT_10, 0U);
    helpers::test_primitive<N>("0b10011101101u;", syntax::TokenType::UINT_2, 0b10011101101U);
    helpers::test_primitive<N>("0o1234567u;", syntax::TokenType::UINT_8, 342'391U);
    helpers::test_primitive<N>("0xFF8a91du;", syntax::TokenType::UINT_16, 0xFF8a91dU);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFu;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned long integer parsing") {
    using N = ast::UnsignedLongIntegerExpression;
    helpers::test_primitive<N>("0ul;", syntax::TokenType::ULINT_10, 0ULL);
    helpers::test_primitive<N>("0b10011101101ul;", syntax::TokenType::ULINT_2, 0b10011101101ULL);
    helpers::test_primitive<N>("0o1234567ul;", syntax::TokenType::ULINT_8, 342'391ULL);
    helpers::test_primitive<N>("0xFF8a91dul;", syntax::TokenType::ULINT_16, 0xFF8a91dULL);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFFul;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Unsigned size integer parsing") {
    using N = ast::USizeIntegerExpression;
    helpers::test_primitive<N>("0uz;", syntax::TokenType::UZINT_10, 0UZ);
    helpers::test_primitive<N>("0b10011101101uz;", syntax::TokenType::UZINT_2, 0b10011101101UZ);
    helpers::test_primitive<N>("0o1234567uz;", syntax::TokenType::UZINT_8, 342'391UZ);
    helpers::test_primitive<N>("0xFF8a91duz;", syntax::TokenType::UZINT_16, 0xFF8a91dUZ);

    helpers::test_primitive<N>(
        "0xFFFFFFFFFFFFFFFFFuz;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::INTEGER_OVERFLOW, 1, 1});
}

TEST_CASE("Byte parsing") {
    using N = ast::ByteExpression;
    helpers::test_primitive<N>("'3';", syntax::TokenType::BYTE, '3');
    helpers::test_primitive<N>("'\\n';", syntax::TokenType::BYTE, '\n');
    helpers::test_primitive<N>("'\\r';", syntax::TokenType::BYTE, '\r');
    helpers::test_primitive<N>("'\\t';", syntax::TokenType::BYTE, '\t');
    helpers::test_primitive<N>("'\\\\';", syntax::TokenType::BYTE, '\\');
    helpers::test_primitive<N>("'\\\'';", syntax::TokenType::BYTE, '\'');
    helpers::test_primitive<N>("'\\\"';", syntax::TokenType::BYTE, '\"');
    helpers::test_primitive<N>("'\\0';", syntax::TokenType::BYTE, '\0');

    helpers::test_primitive<N>(
        "'\\f';",
        {},
        syntax::ParserDiagnostic{syntax::ParserError::UNKNOWN_CHARACTER_ESCAPE, 1, 1});
}

TEST_CASE("Floating point parsing") {
    using N = ast::FloatExpression;
    helpers::test_primitive<N>("1023.0f;", syntax::TokenType::FLOAT, 1023.0f);
    helpers::test_primitive<N>("1023.234612f;", syntax::TokenType::FLOAT, 1023.234612f);

    helpers::test_primitive<N>("1023.234612e234000f;",
                               std::nullopt,
                               syntax::ParserDiagnostic{syntax::ParserError::FLOAT_OVERFLOW, 1, 1});
}

TEST_CASE("Double parsing") {
    using N = ast::DoubleExpression;
    helpers::test_primitive<N>("1023.0;", syntax::TokenType::DOUBLE, 1023.0);
    helpers::test_primitive<N>("1023.234612;", syntax::TokenType::DOUBLE, 1023.234612);
    helpers::test_primitive<N>("1023.234612e234;", syntax::TokenType::DOUBLE, 1023.234612e234);

    helpers::test_primitive<N>(
        "1023.234612e234000;",
        std::nullopt,
        syntax::ParserDiagnostic{syntax::ParserError::DOUBLE_OVERFLOW, 1, 1});
}

TEST_CASE("Bool parsing") {
    using N = ast::BoolExpression;
    helpers::test_primitive<N>("true;", syntax::TokenType::TRUE, true);
    helpers::test_primitive<N>("false;", syntax::TokenType::FALSE, false);
}

} // namespace porpoise::tests

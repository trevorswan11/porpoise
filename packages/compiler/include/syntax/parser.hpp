#pragma once

#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ast/node.hpp"

#include "syntax/lexer.hpp"
#include "syntax/precedence.hpp"
#include "syntax/token.hpp"

namespace porpoise::syntax {

enum class ParserError : u8 {
    UNEXPECTED_TOKEN,
    ENUM_MISSING_VARIANTS,
    MISSING_TRAILING_COMMA,
    MISSING_PREFIX_PARSER,
    INFIX_MISSING_RHS,
    ILLEGAL_IDENTIFIER,
    END_OF_TOKEN_STREAM,
    CONST_DECL_MISSING_VALUE,
    EMPTY_USER_IMPORT,
    ILLEGAL_IMPORT_TYPE,
    USER_IMPORT_MISSING_ALIAS,
    DUPLICATE_DECL_MODIFIER,
    ILLEGAL_DECL_MODIFIERS,
    INTEGER_OVERFLOW,
    FLOAT_OVERFLOW,
    DOUBLE_OVERFLOW,
    UNKNOWN_CHARACTER_ESCAPE,
    MALFORMED_STRING,
    PREFIX_MISSING_OPERAND,
    INDEX_MISSING_EXPRESSION,
    DISCARD_MISSING_DISCARDEE,
    ILLEGAL_BLOCK_STATEMENT,
    EMPTY_LOOP,
    WHILE_MISSING_CONDITION,
    INVALID_STRUCT_MEMBER,
    PACKED_AFTER_STRUCT_KEYWORD,
    EMPTY_STRUCT,
    EXTERN_VALUE_INITIALIZED,
    ILLEGAL_LOOP_NON_BREAK,
    FOR_MISSING_ITERABLES,
    ILLEGAL_FOR_LOOP_CAPTURE,
    EMPTY_FOR_LOOP,
    FOR_ITERABLE_CAPTURE_MISMATCH,
    ILLEGAL_FOR_LOOP_DISCARD,
    EMPTY_WHILE_CONTINUATION,
    EMPTY_WHILE_LOOP,
    IF_MISSING_CONDITION,
    ILLEGAL_IF_BRANCH,
    MISSING_ARRAY_SIZE_TOKEN,
    ILLEGAL_ARRAY_SIZE_TYPE,
    EXPLICIT_ARRAY_SIZE_MISMATCH,
    MATCH_EXPR_MISSING_CONDITION,
    ARMLESS_MATCH_EXPR,
    ILLEGAL_MATCH_ARM,
    ILLEGAL_MATCH_CATCH_ALL,
    FUNCTION_PARAMETER_HAS_DEFAULT_VALUE,
    FUNCTION_PARAMETER_IS_NORETURN,
    ILLEGAL_FUNCTION_DEFINITION,
    ILLEGAL_TYPE_MODIFIER,
    ILLEGAL_EXPLICIT_TYPE,
    EXPLICIT_FN_TYPE_HAS_BODY,
    ILLEGAL_OUTER_SCOPE_TYPE,
    ILLEGAL_FUNCTION_TYPE_MODIFIER,
    ILLEGAL_NORETURN_TYPE_MODIFIER,
    ILLEGAL_VOID_TYPE_MODIFIER,
    ILLEGAL_TYPE_TYPE_MODIFIER,
    COMMA_WITH_MISSING_CALL_ARGUMENT,
    EMPTY_UNION,
    ILLEGAL_DEFERRED_STATEMENT,
    DEFER_MISSING_DEFERREE,
};

using ParserDiagnostic = Diagnostic<ParserError>;

template <typename... Args>
auto make_parser_unexpected(Args&&... args) -> Unexpected<ParserDiagnostic> {
    return make_unexpected<ParserDiagnostic>(std::forward<Args>(args)...);
}

class Parser {
  public:
    using Diagnostics = std::vector<ParserDiagnostic>;
    using PrefixFn    = Expected<mem::Box<ast::Expression>, ParserDiagnostic> (*)(Parser&);
    using InfixFn     = Expected<mem::Box<ast::Expression>, ParserDiagnostic> (*)(
        Parser&, mem::Box<ast::Expression>);

    class Checkpoint {
      public:
        explicit Checkpoint(const Parser& p) noexcept;

      private:
        Lexer::Snapshot snapshot_;
        Token           current_;
        Token           peek_;

        friend class Parser;
    };

    // An RAII checkpoint-rollback transaction
    class Transaction {
      public:
        explicit Transaction(Parser& parser) noexcept : p_{parser}, checkpoint_{parser} {}
        ~Transaction() {
            if (!committed_) { p_.rollback(checkpoint_); }
        }

        // Prevent a rollback from happening at the end of the transaction
        void commit() noexcept { committed_ = true; }

      private:
        Parser&            p_;
        Parser::Checkpoint checkpoint_;
        bool               committed_ = false;
    };

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;

    // Advances the parser, returning the resulting current token.
    // This is a no-op at end of stream.
    auto advance(u8 times = 1) noexcept -> const Token&;
    auto consume() -> std::pair<ast::AST, Diagnostics>;

    auto current_token() const noexcept -> const Token& { return current_token_; }
    auto peek_token() const noexcept -> const Token& { return peek_token_; }

    auto current_token_is(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peek_token_is(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    // Advances the cursor tokens only if the expected token type matches the actual peek token.
    [[nodiscard]] auto expect_peek(TokenType expected)
        -> Expected<std::monostate, ParserDiagnostic>;

    // Indiscriminately returns an error citing the peek token.
    [[nodiscard]] auto peek_error(TokenType expected) -> ParserDiagnostic {
        return tt_mismatch_error(expected, peek_token_);
    }

    auto poll_current_precedence() const noexcept -> std::pair<Precedence, Optional<Binding>>;
    auto poll_peek_precedence() const noexcept -> std::pair<Precedence, Optional<Binding>>;

    [[nodiscard]] auto parse_statement() -> Expected<mem::Box<ast::Statement>, ParserDiagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> Expected<mem::Box<ast::Expression>, ParserDiagnostic>;

    // Assumes that the current token is looking at the start of the expression.
    // The resulting statement can only be a jump, block, or expression statement.
    [[nodiscard]] auto parse_restricted_statement(ParserError error)
        -> Expected<mem::Box<ast::Statement>, ParserDiagnostic>;

    // Parses a restricted statement only if an else token is currently looked at.
    [[nodiscard]] auto try_parse_restricted_alternate(ParserError error)
        -> Expected<Optional<mem::Box<ast::Statement>>, ParserDiagnostic>;

    static constexpr auto poll_prefix_fn(TokenType tt) noexcept -> Optional<const PrefixFn&>;
    static constexpr auto poll_infix_fn(TokenType tt) noexcept -> Optional<const InfixFn&>;

  private:
    static auto tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic;

    // Reverts the parser to the state from the checkpoint.
    auto rollback(const Checkpoint& checkpoint) noexcept -> void {
        lexer_.restore(checkpoint.snapshot_);
        current_token_ = checkpoint.current_;
        peek_token_    = checkpoint.peek_;
    }

  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};
};

} // namespace porpoise::syntax

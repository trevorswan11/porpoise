#pragma once

#include <string_view>
#include <utility>

#include "ast/node.hpp"

#include "syntax/error.hpp"
#include "syntax/lexer.hpp"
#include "syntax/precedence.hpp"
#include "syntax/token.hpp"

#include "result.hpp"
#include "variant.hpp"

namespace porpoise::syntax {

class Parser {
  public:
    using PrefixFn = Result<mem::Box<ast::Expression>, Diagnostic> (*)(Parser&);
    using InfixFn  = Result<mem::Box<ast::Expression>, Diagnostic> (*)(Parser&,
                                                                      mem::Box<ast::Expression>);

    class Checkpoint {
      public:
        explicit Checkpoint(const Parser& parser) noexcept
            : snapshot_{parser.lexer_}, current_{parser.current_token_}, peek_{parser.peek_token_} {
        }

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
        bool               committed_{false};
    };

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;

    // Advances the parser, returning the resulting current token.
    // This is a no-op at end of stream.
    auto advance(u8 times = 1) noexcept -> const Token&;
    auto consume() -> std::pair<ast::AST, Diagnostics>;

    auto get_current_token() const noexcept -> const Token& { return current_token_; }
    auto get_peek_token() const noexcept -> const Token& { return peek_token_; }

    auto current_token_is(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peek_token_is(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    // Advances the cursor tokens only if the expected token type matches the actual peek token.
    [[nodiscard]] auto expect_peek(TokenType expected) -> Result<Unit, Diagnostic>;

    // Indiscriminately returns an error citing the peek token.
    [[nodiscard]] auto peek_error(TokenType expected) -> Diagnostic;

    auto get_current_precedence() const noexcept -> std::pair<Precedence, opt::Option<Binding>>;
    auto get_peek_precedence() const noexcept -> std::pair<Precedence, opt::Option<Binding>>;

    [[nodiscard]] auto parse_statement(bool require_semicolon)
        -> Result<mem::Box<ast::Statement>, Diagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> Result<mem::Box<ast::Expression>, Diagnostic>;

    // Assumes that the current token is looking at the start of the expression.
    // The resulting statement can only be a jump, block, or expression statement.
    [[nodiscard]] auto parse_restricted_statement(Error error, bool require_semicolon = true)
        -> Result<mem::Box<ast::Statement>, Diagnostic>;

    // Parses a restricted statement only if an else token is currently looked at.
    [[nodiscard]] auto try_parse_restricted_alternate(Error error, bool require_semicolon = true)
        -> Result<mem::NullableBox<ast::Statement>, Diagnostic>;

    static auto try_get_prefix_fn(TokenType tt) noexcept -> opt::Option<PrefixFn>;
    static auto try_get_poll_infix_fn(TokenType tt) noexcept -> opt::Option<InfixFn>;

  private:
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

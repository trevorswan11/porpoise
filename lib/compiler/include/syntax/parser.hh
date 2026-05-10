#pragma once

#include <string_view>
#include <utility>

#include "ast/forest.hh"
#include "ast/handle.hh"
#include "ast/meta.hh"

#include "option.hh"
#include "syntax/error.hh"
#include "syntax/lexer.hh"
#include "syntax/precedence.hh"
#include "syntax/token.hh"

#include "result.hh"
#include "variant.hh"

namespace porpoise::ast { class Forest; } // namespace porpoise::ast

namespace porpoise::syntax {

class Parser {
  public:
    using PrefixFn = Result<ast::ExpressionHandle, Diagnostic> (*)(Parser&);
    using InfixFn  = Result<ast::ExpressionHandle, Diagnostic> (*)(Parser&, ast::ExpressionHandle);

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
        explicit Transaction(Parser& parser) noexcept : parser_{parser}, checkpoint_{parser} {}
        ~Transaction() {
            if (!committed_) { parser_.rollback(checkpoint_); }
        }

        // Prevent a rollback from happening at the end of the transaction
        void commit() noexcept { committed_ = true; }

      private:
        Parser&            parser_;
        Parser::Checkpoint checkpoint_;
        bool               committed_{false};
    };

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept { reset(input); }

    auto reset(std::string_view input = {}) noexcept -> void;

    // Advances the parser, returning the resulting current token.
    // This is a no-op at end of stream.
    auto advance(u8 times = 1) noexcept -> const Token&;

    // Fills the AST with the parser's output, clearing it before use
    auto consume(ast::Forest& forest) -> Diagnostics;

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
        -> Result<ast::StatementHandle, Diagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> Result<ast::ExpressionHandle, Diagnostic>;

    // Assumes that the current token is looking at the start of the expression.
    // The resulting statement can only be a jump, block, or expression statement.
    [[nodiscard]] auto parse_restricted_statement(Error error, bool require_semicolon = true)
        -> Result<ast::StatementHandle, Diagnostic>;

    // Parses a restricted statement only if an else token is currently looked at.
    [[nodiscard]] auto try_parse_restricted_alternate(Error error, bool require_semicolon = true)
        -> Result<opt::Option<ast::StatementHandle>, Diagnostic>;

    static auto try_get_prefix_fn(TokenType tt) noexcept -> opt::Option<PrefixFn>;
    static auto try_get_poll_infix_fn(TokenType tt) noexcept -> opt::Option<InfixFn>;

    MAKE_NON_CONST_OPTIONAL_UNPACKER(forest, ast::Forest&, forest_, *)

    template <ast::traits::ASTNode N>
    [[nodiscard]] constexpr auto get_node(ast::NodeID id) -> const N& {
        return std::get<N>((*forest_)[id]);
    }

    [[nodiscard]] auto get_location_of(ast::NodeID id) -> SourceLocation;
    [[nodiscard]] auto get_location_of(ast::ExplicitTypeID id) -> SourceLocation;

    // Adds an expression to the forest and returns its handle
    template <ast::traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_expr(const syntax::Token& start_token, Data&& data) {
        return add_node<ast::ExpressionHandle>(start_token, std::forward<Data>(data));
    }

    // Adds a statement to the forest and returns its handle
    template <ast::traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_stmt(const syntax::Token& start_token, Data&& data) {
        return add_node<ast::StatementHandle>(start_token, std::forward<Data>(data));
    }

    // Adds a node to the forest and casts the result to the requested handle
    template <typename Handle, ast::traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> Handle {
        return Handle{forest_->add_node(start_token, std::forward<Data>(data))};
    }

    // Helper for type-forest insertion, reducing a layer of call-site indirection
    template <ast::traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto add_type(const syntax::Token& start_token,
                                          ast::TypeModifier    mod,
                                          Data&&               data) -> ast::ExplicitTypeID {
        return forest_->add_type(start_token, mod, std::forward<Data>(data));
    }

  private:
    // Reverts the parser to the state from the checkpoint.
    auto rollback(const Checkpoint& checkpoint) noexcept -> void {
        lexer_.restore(checkpoint.snapshot_);
        current_token_ = checkpoint.current_;
        peek_token_    = checkpoint.peek_;
    }

  private:
    std::string_view          input_;
    Lexer                     lexer_{};
    Token                     current_token_{};
    Token                     peek_token_{};
    opt::Option<ast::Forest&> forest_;
};

} // namespace porpoise::syntax

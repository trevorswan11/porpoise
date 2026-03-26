#include "ast/statements/import.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

LibraryImport::LibraryImport(mem::Box<IdentifierExpression>           name,
                             Optional<mem::Box<IdentifierExpression>> alias) noexcept
    : name_{std::move(name)}, alias_{std::move(alias)} {}
LibraryImport::~LibraryImport() = default;

auto LibraryImport::is_equal(const LibraryImport& rhs) const noexcept -> bool {
    return *name_ == *rhs.name_ && optional::unsafe_eq<IdentifierExpression>(alias_, rhs.alias_);
}

FileImport::FileImport(mem::Box<StringExpression>     file,
                       mem::Box<IdentifierExpression> alias) noexcept
    : file_{std::move(file)}, alias_{std::move(alias)} {}
FileImport::~FileImport() = default;

auto FileImport::is_equal(const FileImport& rhs) const noexcept -> bool {
    return *file_ == *rhs.file_ && *alias_ == *rhs.alias_;
}

ImportStatement::ImportStatement(const syntax::Token& start_token, ImportVariant imported) noexcept
    : StmtBase{start_token}, imported_{std::move(imported)} {}
ImportStatement::~ImportStatement() = default;

auto ImportStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImportStatement::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::variant<mem::Box<IdentifierExpression>, mem::Box<StringExpression>> imported_core;
    if (parser.peek_token_is(syntax::TokenType::IDENT)) {
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        imported_core = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    } else if (parser.peek_token_is(syntax::TokenType::STRING)) {
        TRY(parser.expect_peek(syntax::TokenType::STRING));
        auto string = downcast<StringExpression>(TRY(StringExpression::parse(parser)));

        if (string->get_value().empty()) {
            return make_parser_unexpected(syntax::ParserError::EMPTY_USER_IMPORT,
                                          string->get_token());
        }
        imported_core = std::move(string);
    } else {
        return make_parser_unexpected(syntax::ParserError::ILLEGAL_IMPORT_TYPE,
                                      parser.peek_token());
    }

    Optional<mem::Box<IdentifierExpression>> imported_alias;
    if (parser.peek_token_is(syntax::TokenType::AS)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));

        imported_alias = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    } else if (std::holds_alternative<mem::Box<StringExpression>>(imported_core)) {
        return make_parser_unexpected(syntax::ParserError::USER_IMPORT_MISSING_ALIAS, start_token);
    }

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }

    return mem::make_box<ImportStatement>(
        start_token,
        std::visit(Overloaded{[&](mem::Box<IdentifierExpression>& ident) -> ImportVariant {
                                  return LibraryImport{std::move(ident), std::move(imported_alias)};
                              },
                              [&](mem::Box<StringExpression>& string) -> ImportVariant {
                                  return FileImport{std::move(string), std::move(*imported_alias)};
                              }},
                   imported_core));
}

auto ImportStatement::has_alias() const noexcept -> bool {
    return std::visit(Overloaded{
                          [](const LibraryImport& v) { return v.has_alias(); },
                          [](const FileImport&) { return true; },
                      },
                      imported_);
}

auto ImportStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted         = as<ImportStatement>(other);
    const auto& other_imported = casted.imported_;
    return std::visit(Overloaded{
                          [&other_imported](const LibraryImport& v) {
                              return v == std::get<LibraryImport>(other_imported);
                          },
                          [&other_imported](const FileImport& v) {
                              return v == std::get<FileImport>(other_imported);
                          },
                      },
                      imported_) &&
           public_ == casted.public_;
}

} // namespace porpoise::ast

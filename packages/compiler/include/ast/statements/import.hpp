#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class StringExpression;

class ModuleImport {
  public:
    explicit ModuleImport(Box<IdentifierExpression>           name,
                          Optional<Box<IdentifierExpression>> alias) noexcept;
    ~ModuleImport();

    MAKE_AST_COPY_MOVE(ModuleImport)

    MAKE_GETTER(name, const IdentifierExpression&, *)
    MAKE_OPTIONAL_UNPACKER(alias, IdentifierExpression, alias_, **)

    MAKE_EQ_DELEGATION(ModuleImport)

  private:
    Box<IdentifierExpression>           name_;
    Optional<Box<IdentifierExpression>> alias_;
};

class UserImport {
  public:
    explicit UserImport(Box<StringExpression> file, Box<IdentifierExpression> alias) noexcept;
    ~UserImport();

    MAKE_AST_COPY_MOVE(UserImport)

    MAKE_GETTER(file, const StringExpression&, *)
    MAKE_GETTER(alias, const IdentifierExpression&, *)

    MAKE_EQ_DELEGATION(UserImport)

  private:
    Box<StringExpression>     file_;
    Box<IdentifierExpression> alias_;
};

class ImportStatement : public StmtBase<ImportStatement> {
  public:
    static constexpr auto KIND = NodeKind::IMPORT_STATEMENT;

    using ImportVariant = std::variant<ModuleImport, UserImport>;

  public:
    explicit ImportStatement(const Token& start_token, ImportVariant imported) noexcept;
    ~ImportStatement() override;

    MAKE_AST_COPY_MOVE(ImportStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    MAKE_VARIANT_UNPACKER(module_import, ModuleImport, ModuleImport, imported_, std::get)
    MAKE_VARIANT_UNPACKER(user_import, UserImport, UserImport, imported_, std::get)

    // Prefer using the matcher (`match`) over this convenience check
    [[nodiscard]] auto has_alias() const noexcept -> bool;
    MAKE_VARIANT_MATCHER(imported_)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    ImportVariant imported_;
};

} // namespace porpoise::ast

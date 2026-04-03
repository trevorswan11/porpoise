#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class StringExpression;

class LibraryImport {
  public:
    LibraryImport(mem::Box<IdentifierExpression>           name,
                  Optional<mem::Box<IdentifierExpression>> alias) noexcept;
    ~LibraryImport();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(LibraryImport)

    MAKE_GETTER(name, const IdentifierExpression&, *)
    MAKE_OPTIONAL_UNPACKER(alias, IdentifierExpression, alias_, **)

    MAKE_EQ_DELEGATION(LibraryImport)

  private:
    mem::Box<IdentifierExpression>           name_;
    Optional<mem::Box<IdentifierExpression>> alias_;
};

class FileImport {
  public:
    FileImport(mem::Box<StringExpression> file, mem::Box<IdentifierExpression> alias) noexcept;
    ~FileImport();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(FileImport)

    MAKE_GETTER(file, const StringExpression&, *)
    MAKE_GETTER(alias, const IdentifierExpression&, *)

    MAKE_EQ_DELEGATION(FileImport)

  private:
    mem::Box<StringExpression>     file_;
    mem::Box<IdentifierExpression> alias_;
};

class ImportStatement : public StmtBase<ImportStatement> {
  public:
    static constexpr auto KIND = NodeKind::IMPORT_STATEMENT;

    using ImportVariant = std::variant<LibraryImport, FileImport>;

  public:
    ImportStatement(const syntax::Token& start_token, ImportVariant imported) noexcept;
    ~ImportStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ImportStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_VARIANT_UNPACKER(library_import, LibraryImport, LibraryImport, imported_, std::get)
    MAKE_VARIANT_UNPACKER(file_import, FileImport, FileImport, imported_, std::get)

    // Prefer using the matcher (`match`) over this convenience check
    [[nodiscard]] auto has_alias() const noexcept -> bool;
    MAKE_VARIANT_MATCHER(imported_)

    auto               mark_public() const noexcept -> void { public_ = true; }
    [[nodiscard]] auto is_public() const noexcept -> bool { return public_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    ImportVariant imported_;
    mutable bool  public_{false}; // Updated in sema
};

} // namespace porpoise::ast

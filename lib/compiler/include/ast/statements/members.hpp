#pragma once

#include "ast/node.hpp"
#include "ast/statements/declaration.hpp"
#include "ast/statements/import.hpp"
#include "ast/statements/using.hpp"

#include "iterator.hpp"
#include "variant.hpp"

namespace porpoise::ast {

class Members {
  public:
    using Member =
        std::variant<mem::Box<DeclStatement>, mem::Box<ImportStatement>, mem::Box<UsingStatement>>;

  public:
    MAKE_ITERATOR(MemberList, std::vector<Member>, members_);

  public:
    explicit Members(MemberList members) noexcept : members_{std::move(members)} {}
    ~Members() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Members)

    // Dispatches the active variant to its rightful visitor
    static auto accept(const Member& member, Visitor& v) -> void;

    // Parses and validates the resulting member using the std::visit-acceptable validator
    //
    // The validator need not check if the node fits in the Member variant
    template <typename MemberValidator>
    [[nodiscard]] static auto parse(syntax::Parser& parser, MemberValidator&& validator)
        -> Result<Members, syntax::Diagnostic> {
        MemberList members;
        while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            parser.advance();
            auto parsed_member = TRY(parser.parse_statement(true));

            // Downcast the parsed member into the specific member variant and check
            auto member = TRY(deconstruct_member(std::move(parsed_member)));
            if (!std::visit(std::forward<MemberValidator>(validator), member)) {
                return make_syntax_err(
                    syntax::Error::INVALID_MEMBER,
                    std::visit([](const auto& m) -> auto& { return m->get_token(); }, member));
            }
            members.emplace_back(std::move(member));
        }
        return Members{std::move(members)};
    }

    static auto validate_struct_decl(const DeclStatement& decl) noexcept -> bool;
    static auto validate_non_struct_decl(const DeclStatement& decl) noexcept -> bool;

    MAKE_EQ_DELEGATION(Members)

  private:
    [[nodiscard]] static auto deconstruct_member(mem::Box<Statement> member)
        -> Result<Member, syntax::Diagnostic> {
        switch (member->get_kind()) {
        case NodeKind::DECL_STATEMENT:
            return Member{Node::downcast<DeclStatement>(std::move(member))};
        case NodeKind::USING_STATEMENT:
            return Member{Node::downcast<UsingStatement>(std::move(member))};
        case NodeKind::IMPORT_STATEMENT:
            return Member{Node::downcast<ImportStatement>(std::move(member))};
        default: return make_syntax_err(syntax::Error::INVALID_MEMBER, member->get_token());
        }
    }

  private:
    MemberList members_;
};

} // namespace porpoise::ast

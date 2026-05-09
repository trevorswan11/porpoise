#include "ast/id.hh"

#include "enum.hh"

namespace porpoise::ast {

namespace {

constexpr auto NODE_NAMES = [] {
    EnumMap<NodeKind, std::string_view> names{"expression"};

    names[NodeKind::ENUM_EXPRESSION]     = "enum";
    names[NodeKind::FUNCTION_EXPRESSION] = "function";
    names[NodeKind::UNION_EXPRESSION]    = "union";
    names[NodeKind::STRUCT_EXPRESSION]   = "struct";

    for (const auto kind : enum_range<NodeKind::BLOCK_STATEMENT, NodeKind::USING_STATEMENT>()) {
        names[kind] = "statement";
    }
    return names;
}();

} // namespace

auto NodeID::display_name() const noexcept -> std::string_view { return NODE_NAMES[get_kind()]; }

namespace {

using Modifier           = TypeModifier::Modifier;
using ModifierMapping    = std::pair<syntax::TokenType, Modifier>;
constexpr auto MODIFIERS = [] {
    using TokenType = syntax::TokenType;
    EnumMap<TokenType, Modifier> modifiers{Modifier::VALUE};
    modifiers[TokenType::BW_AND]   = Modifier::REF;
    modifiers[TokenType::AND_MUT]  = Modifier::MUT_REF;
    modifiers[TokenType::STAR]     = Modifier::PTR;
    modifiers[TokenType::STAR_MUT] = Modifier::MUT_PTR;
    modifiers[TokenType::VOLATILE] = Modifier::VOLATILE;
    return modifiers;
}();

} // namespace

TypeModifier::TypeModifier(const syntax::Token& tok) noexcept : underlying_{MODIFIERS[tok.type]} {}

} // namespace porpoise::ast

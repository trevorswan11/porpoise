#include "ast/id.hh"

#include "enum.hh"

namespace porpoise::ast {

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

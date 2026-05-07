#include "ast/expressions/type_modifiers.hpp"

#include "enum.hpp"

namespace porpoise::ast {

namespace {

using Modifier           = TypeModifier::Modifier;
using ModifierMapping    = std::pair<syntax::TokenType, Modifier>;
constexpr auto MODIFIERS = [] {
    using TokenType = syntax::TokenType;
    EnumMap<TokenType, opt::Option<Modifier>> modifiers{opt::none};
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

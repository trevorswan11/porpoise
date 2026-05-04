#include <algorithm>

#include "ast/expressions/type_modifiers.hpp"

namespace porpoise::ast {

namespace {

using Modifier                 = TypeModifier::Modifier;
using ModifierMapping          = std::pair<syntax::TokenType, Modifier>;
constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
    {syntax::TokenType::BW_AND, Modifier::REF},
    {syntax::TokenType::AND_MUT, Modifier::MUT_REF},
    {syntax::TokenType::STAR, Modifier::PTR},
    {syntax::TokenType::STAR_MUT, Modifier::MUT_PTR},
    {syntax::TokenType::VOLATILE, Modifier::VOLATILE},
});

} // namespace

TypeModifier::TypeModifier(const syntax::Token& tok) noexcept {
    const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
    if (it != LEGAL_MODIFIERS.end()) { underlying_ = it->second; }
}

} // namespace porpoise::ast

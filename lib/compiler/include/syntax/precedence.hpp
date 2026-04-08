#pragma once

#include <algorithm>
#include <array>

#include "syntax/token.hpp"

#include "optional.hpp"
#include "types.hpp"

namespace porpoise::syntax {

enum class Precedence : u8 {
    LOWEST,
    ASSIGNMENT       = 10,
    BOOL_AND_OR      = 20,
    BOOL_EQUIV       = 30,
    BOOL_LT_GT       = 40,
    ADD_SUB          = 50,
    MUL_DIV          = 60,
    EXPONENT         = 70,
    PREFIX           = 80,
    RANGE            = 90,
    SCOPE_RESOLUTION = 100,
    GROUP_CALL_IDX   = 110,
    INITIALIZATION   = 120,
};

struct Binding {
    TokenType  type;
    Precedence precedence;
    bool       right_assoc{false};
};

constexpr auto ALL_BINDINGS = [] {
    auto all_bindings = std::to_array<Binding>({
        {TokenType::PLUS, Precedence::ADD_SUB},
        {TokenType::MINUS, Precedence::ADD_SUB},
        {TokenType::STAR, Precedence::MUL_DIV},
        {TokenType::SLASH, Precedence::MUL_DIV},
        {TokenType::PERCENT, Precedence::MUL_DIV},
        {TokenType::BOOLEAN_AND, Precedence::BOOL_AND_OR},
        {TokenType::BOOLEAN_OR, Precedence::BOOL_AND_OR},
        {TokenType::EQ, Precedence::BOOL_EQUIV},
        {TokenType::NEQ, Precedence::BOOL_EQUIV},
        {TokenType::LT, Precedence::BOOL_LT_GT},
        {TokenType::LT_EQ, Precedence::BOOL_LT_GT},
        {TokenType::GT, Precedence::BOOL_LT_GT},
        {TokenType::GT_EQ, Precedence::BOOL_LT_GT},
        {TokenType::BW_AND, Precedence::MUL_DIV},
        {TokenType::BW_OR, Precedence::ADD_SUB},
        {TokenType::XOR, Precedence::ADD_SUB},
        {TokenType::SHR, Precedence::MUL_DIV},
        {TokenType::SHL, Precedence::MUL_DIV},
        {TokenType::LPAREN, Precedence::GROUP_CALL_IDX},
        {TokenType::LBRACKET, Precedence::GROUP_CALL_IDX},
        {TokenType::DOT_DOT, Precedence::RANGE},
        {TokenType::DOT_DOT_EQ, Precedence::RANGE},
        {TokenType::ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::PLUS_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::MINUS_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::STAR_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::SLASH_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::PERCENT_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::BW_AND_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::BW_OR_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::SHL_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::SHR_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::NOT_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::XOR_ASSIGN, Precedence::ASSIGNMENT, true},
        {TokenType::DOT, Precedence::SCOPE_RESOLUTION},
        {TokenType::COLON_COLON, Precedence::SCOPE_RESOLUTION},
        {TokenType::LBRACE, Precedence::INITIALIZATION},
    });

    std::ranges::sort(all_bindings, {}, &Binding::type);
    return all_bindings;
}();

constexpr auto get_binding(TokenType tt) noexcept -> Optional<Binding> {
    const auto it = std::ranges::lower_bound(ALL_BINDINGS, tt, {}, &Binding::type);
    if (it == ALL_BINDINGS.end() || it->type != tt) { return std::nullopt; }
    return Optional<Binding>{*it};
}

} // namespace porpoise::syntax

#pragma once

#include "syntax/token.hpp"

#include "option.hpp"
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
    TYPE             = 100,
    SCOPE_RESOLUTION = 110,
    GROUP_CALL_IDX   = 120,
    INITIALIZATION   = 130,
};

struct Binding {
    Precedence precedence;
    bool       right_assoc{false};

    [[nodiscard]] static auto try_get_from(TokenType tt) noexcept -> opt::Option<Binding>;
};

} // namespace porpoise::syntax

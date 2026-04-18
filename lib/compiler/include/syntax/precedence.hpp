#pragma once

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
    Precedence precedence;
    bool       right_assoc{false};

    [[nodiscard]] static auto try_get_from(TokenType tt) noexcept -> Optional<Binding>;
};

} // namespace porpoise::syntax

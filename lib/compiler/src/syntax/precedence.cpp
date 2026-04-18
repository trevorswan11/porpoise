#include <array>

#include "syntax/precedence.hpp"

namespace porpoise::syntax {

constexpr auto ALL_BINDINGS = [] {
    using enum TokenType;
    std::array<Optional<Binding>, TOKEN_TYPE_COUNT> bindings;

    bindings[static_cast<usize>(PLUS)]           = {Precedence::ADD_SUB};
    bindings[static_cast<usize>(MINUS)]          = {Precedence::ADD_SUB};
    bindings[static_cast<usize>(STAR)]           = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(SLASH)]          = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(PERCENT)]        = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(BOOLEAN_AND)]    = {Precedence::BOOL_AND_OR};
    bindings[static_cast<usize>(BOOLEAN_OR)]     = {Precedence::BOOL_AND_OR};
    bindings[static_cast<usize>(EQ)]             = {Precedence::BOOL_EQUIV};
    bindings[static_cast<usize>(NEQ)]            = {Precedence::BOOL_EQUIV};
    bindings[static_cast<usize>(LT)]             = {Precedence::BOOL_LT_GT};
    bindings[static_cast<usize>(LT_EQ)]          = {Precedence::BOOL_LT_GT};
    bindings[static_cast<usize>(GT)]             = {Precedence::BOOL_LT_GT};
    bindings[static_cast<usize>(GT_EQ)]          = {Precedence::BOOL_LT_GT};
    bindings[static_cast<usize>(BW_AND)]         = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(BW_OR)]          = {Precedence::ADD_SUB};
    bindings[static_cast<usize>(XOR)]            = {Precedence::ADD_SUB};
    bindings[static_cast<usize>(SHR)]            = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(SHL)]            = {Precedence::MUL_DIV};
    bindings[static_cast<usize>(LPAREN)]         = {Precedence::GROUP_CALL_IDX};
    bindings[static_cast<usize>(LBRACKET)]       = {Precedence::GROUP_CALL_IDX};
    bindings[static_cast<usize>(DOT_DOT)]        = {Precedence::RANGE};
    bindings[static_cast<usize>(DOT_DOT_EQ)]     = {Precedence::RANGE};
    bindings[static_cast<usize>(ASSIGN)]         = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(PLUS_ASSIGN)]    = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(MINUS_ASSIGN)]   = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(STAR_ASSIGN)]    = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(SLASH_ASSIGN)]   = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(PERCENT_ASSIGN)] = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(BW_AND_ASSIGN)]  = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(BW_OR_ASSIGN)]   = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(SHL_ASSIGN)]     = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(SHR_ASSIGN)]     = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(NOT_ASSIGN)]     = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(XOR_ASSIGN)]     = {Precedence::ASSIGNMENT, true};
    bindings[static_cast<usize>(DOT)]            = {Precedence::SCOPE_RESOLUTION};
    bindings[static_cast<usize>(COLON_COLON)]    = {Precedence::SCOPE_RESOLUTION};
    bindings[static_cast<usize>(LBRACE)]         = {Precedence::INITIALIZATION};

    return bindings;
}();

auto Binding::try_get_from(TokenType tt) noexcept -> Optional<Binding> {
    return ALL_BINDINGS[static_cast<usize>(tt)];
}

} // namespace porpoise::syntax

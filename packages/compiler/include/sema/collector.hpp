#pragma once

#include "ast/visitor.hpp"

namespace porpoise::sema {

// A very shallow AST-consumer for pass 1.
// - Collects top-level declarations only
// - Performs 0 type-checking
// - Does not verify undeclared identifier use
class SymbolCollector : public ast::Visitor {};

} // namespace porpoise::sema

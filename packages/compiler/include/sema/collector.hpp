#pragma once

#include "ast/visitor.hpp"

namespace porpoise::sema {

class SymbolCollector : public ast::Visitor {};

} // namespace porpoise::sema

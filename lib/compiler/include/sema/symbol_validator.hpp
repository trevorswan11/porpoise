#pragma once

#include "ast/visitor.hpp"

#include "sema/context.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"
#include "sema/type.hpp"

namespace porpoise::sema {

// Verifies that all referenced symbols exist
class SymbolValidator : public ast::Visitor {
  public:
    MAKE_AST_VISITOR_OVERRIDES()

  private:
    [[maybe_unused]] Context ctx_;
};

} // namespace porpoise::sema

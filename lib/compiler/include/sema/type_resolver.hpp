#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/symbol.hpp"
#include "sema/type.hpp"

namespace porpoise::sema {

class Analyzer;
class TypePool;

// You aren't going to believe what this does (Pass 2)
class TypeResolver : public ast::Visitor {
  public:
    TypeResolver(Analyzer& analyzer, SymbolTableStack& stack) noexcept;

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    [[maybe_unused]] Analyzer&         analyzer_;
    [[maybe_unused]] TypePool&         pool_;
    [[maybe_unused]] SymbolTableStack& stack_;
    opt::Option<Type&>                 last_type_;
};

} // namespace porpoise::sema

#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/context.hpp"

namespace porpoise::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver : public ast::Visitor {
  public:
    TypeResolver(const Context& ctx) noexcept : ctx_{ctx} {}

    static auto resolve_types(mod::Module& module, const Context& ctx) -> mod::ModuleState;

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    [[maybe_unused]] Context ctx_;
    opt::Option<Type&>       last_type_;
};

} // namespace porpoise::sema

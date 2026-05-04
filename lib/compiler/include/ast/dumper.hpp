#pragma once

#include <ostream>

#include <fmt/ostream.h>

#include "ast/statements/members.hpp"
#include "ast/visitor.hpp"

#include "indent.hpp"

namespace porpoise::ast {

class ExplicitType;

class ASTDumper : public Visitor {
  public:
    explicit ASTDumper(std::ostream& out) : out_{out} {}

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    template <typename T, typename Func> void dump_container(const T& container, Func&& func) {
        for (auto it = container.begin(); it != container.end(); ++it) {
            Indent::Guard g{indent_, std::next(it) == container.end()};
            std::forward<Func>(func)(*it);
        }
    }

    template <typename T> void dump_node_list(const T& list) {
        dump_container(list, [this](const auto& node) {
            fmt::print(out_, "{}", indent_.current_branch());
            unwrap_and_accept(node);
        });
    }

    template <> void dump_node_list<Members>(const Members& list) {
        dump_container(list, [this](const auto& node) {
            fmt::print(out_, "{}", indent_.current_branch());
            Members::accept(node, *this);
        });
    }

  private:
    std::ostream& out_;
    Indent        indent_;
};

} // namespace porpoise::ast

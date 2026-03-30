#pragma once

#include <ostream>

#include <fmt/ostream.h>
#include <type_traits>

#include "ast/visitor.hpp"

#include "indent.hpp"
#include "memory.hpp"

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
            if constexpr (mem::is_box_v<std::remove_cvref_t<decltype(node)>>) {
                node->accept(*this);
            } else {
                node.accept(*this);
            }
        });
    }

  private:
    std::ostream& out_;
    Indent        indent_;
};

} // namespace porpoise::ast

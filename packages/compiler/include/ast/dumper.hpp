#pragma once

#include <ostream>

#include <fmt/ostream.h>

#include "ast/visitor.hpp"

#include "indent.hpp"

namespace conch::ast {

class ExplicitType;

class ASTDumper : public Visitor {
  public:
    explicit ASTDumper(std::ostream& out) : out_{out} {}

    AST_VISITOR_OVERRIDES()

  private:
    auto dump_explicit_type(const ExplicitType& type, bool print_branch) -> void;

    template <typename T> void dump_list(const T& container) {
        for (auto it = container.begin(); it != container.end(); ++it) {
            const Indent::Guard g{indent_, std::next(it) == container.end()};
            fmt::print(out_, "{}", indent_.current_branch());
            (*it)->accept(*this);
        }
    }

  private:
    std::ostream& out_;
    Indent        indent_;
};

} // namespace conch::ast

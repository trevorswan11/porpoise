#pragma once

#include <ostream>

#include <fmt/ostream.h>

#include "ast/ast.hh"
#include "ast/id.hh"
#include "ast/nodes.hh"

#include "assert.hh"
#include "indent.hh"

namespace porpoise::ast {

class ASTDumper {
  public:
    explicit ASTDumper(std::ostream& out, const AST& tree) : out_{out}, tree_{tree} {}

    auto dump(const NodeID& id) -> void {
        std::visit([&](const auto& data) { visit(id, data); }, tree_[id]);
    }

    template <NodeKind... Kinds> auto dump(const Handle<Kinds...>& id) -> void {
        ASSERT(id.is_valid(), "Attempt to dump invalid handle");
        std::visit([&](const auto& data) { visit(*id, data); }, tree_[*id]);
    }

    auto dump(const ExplicitTypeID& id) -> void {
        fmt::println(out_, "ExplicitType (modifier: {})", id.get_modifier());
        const Indent::Guard g{indent_, true};
        std::visit([&](const auto& data) { visit(id, data); }, tree_[id]);
    }

  private:
    AST_VISITOR_DEF_GEN()

    template <typename T, typename Func> void dump_container(const T& container, Func&& func) {
        for (auto it = container.begin(); it != container.end(); ++it) {
            Indent::Guard g{indent_, std::next(it) == container.end()};
            std::forward<Func>(func)(*it);
        }
    }

    template <typename T> void dump_node_list(const T& list) {
        dump_container(list, [this](const auto& node_handle) {
            fmt::print(out_, "{}", indent_.current_branch());
            dump(*node_handle);
        });
    }

    template <> void dump_node_list<Members>(const Members& list) {
        dump_container(list, [this](const Members::Member& member_handle) {
            fmt::print(out_, "{}", indent_.current_branch());
            dump(*member_handle);
        });
    }

  private:
    std::ostream& out_;
    const AST&    tree_;
    Indent        indent_;
};

} // namespace porpoise::ast

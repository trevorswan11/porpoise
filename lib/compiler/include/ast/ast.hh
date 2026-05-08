#pragma once

#include <vector>

#include "ast/id.hh"
#include "ast/nodes.hh"

#include "diagnostic.hh"
#include "iterator.hh"

namespace porpoise::ast {

class AST {
  public:
    MAKE_ITERATOR(RootIDs, std::vector<NodeID>, roots_)

  public:
    template <traits::ASTNode Data>
    constexpr auto add_node(syntax::TokenType start_token_type, Data&& data) -> NodeID {
        constexpr auto kind  = traits::NodeKindOf<Data>::value();
        const auto     index = static_cast<u64>(pool_.size());

        pool_.emplace_back(std::forward<Data>(data));
        return NodeID{kind, start_token_type, index};
    }

    constexpr auto               add_root(NodeID id) -> void { roots_.push_back(id); }
    [[nodiscard]] constexpr auto location_of(NodeID id) const noexcept -> const SourceLocation& {
        return locations_[id.get_index()];
    }

    [[nodiscard]] constexpr auto operator[](NodeID id) const -> std::pair<NodeID, const NodeData&> {
        return {id, pool_[id.get_index()]};
    }

    constexpr auto clear() noexcept -> void {
        roots_.clear();
        pool_.clear();
    }

  private:
    RootIDs                     roots_;
    std::vector<NodeData>       pool_;
    std::vector<SourceLocation> locations_;
};

} // namespace porpoise::ast

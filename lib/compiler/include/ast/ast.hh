#pragma once

#include <vector>

#include "ast/id.hh"
#include "ast/nodes.hh"

#include "diagnostic.hh"
#include "iterator.hh"

namespace porpoise::ast {

class AST {
  public:
    MAKE_UNALIASED_ITERATOR(std::vector<NodeID>, node_roots_)

  public:
    constexpr auto add_root(NodeID id) -> void { node_roots_.push_back(id); }

    template <traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> NodeID {
        constexpr auto kind  = traits::NodeKindOf<Data>::value();
        const auto     index = static_cast<u64>(node_pool_.size());

        node_pool_.emplace_back(std::forward<Data>(data));
        node_locations_.emplace_back(
            ::porpoise::traits::SourceInfo<syntax::Token>::get(start_token));
        return NodeID{kind, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(NodeID id) const noexcept -> const SourceLocation& {
        return node_locations_[id.get_index()];
    }

    [[nodiscard]] constexpr auto operator[](NodeID id) const -> const NodeData& {
        return node_pool_[id.get_index()];
    }

    template <traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, TypeModifier mod, Data&& data) -> ExplicitTypeID {
        constexpr auto kind  = traits::ExplicitTypeKindOf<Data>::value();
        const auto     index = static_cast<u64>(type_pool_.size());

        type_pool_.emplace_back(std::forward<Data>(data));
        type_locations_.emplace_back(
            ::porpoise::traits::SourceInfo<syntax::Token>::get(start_token));
        return ExplicitTypeID{kind, mod, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(ExplicitTypeID id) const noexcept
        -> const SourceLocation& {
        return type_locations_[id.get_index()];
    }

    [[nodiscard]] constexpr auto operator[](ExplicitTypeID id) const -> const TypeData& {
        return type_pool_[id.get_index()];
    }

    constexpr auto clear() noexcept -> void {
        node_roots_.clear();
        node_pool_.clear();
        node_locations_.clear();

        type_roots_.clear();
        type_pool_.clear();
        type_locations_.clear();
    }

  private:
    std::vector<NodeID>         node_roots_;
    std::vector<NodeData>       node_pool_;
    std::vector<SourceLocation> node_locations_;

    std::vector<ExplicitTypeID> type_roots_;
    std::vector<TypeData>       type_pool_;
    std::vector<SourceLocation> type_locations_;
};

} // namespace porpoise::ast

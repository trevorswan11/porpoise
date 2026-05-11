#pragma once

#include <vector>

#include "ast/meta.hh"
#include "ast/visitor.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "iterator.hh"

namespace porpoise {

namespace sema { class Type; } // namespace sema

namespace ast {

template <typename ID, typename Data> struct DataPoolBase {
    std::vector<Data>           pool;
    std::vector<SourceLocation> locations;

    constexpr auto clear() noexcept -> void {
        pool.clear();
        locations.clear();
    }

    constexpr auto emplace_back(const syntax::Token& start_token, Data&& data) -> u64 {
        const auto index = static_cast<u64>(pool.size());
        pool.emplace_back(std::forward<Data>(data));
        locations.emplace_back(::porpoise::traits::SourceInfo<syntax::Token>::get(start_token));
        return index;
    }

    [[nodiscard]] constexpr auto location_of(ID id) const noexcept -> const SourceLocation& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return locations[id.get_index()];
    }

    [[nodiscard]] constexpr auto operator[](ID id) const noexcept -> const Data& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return pool[id.get_index()];
    }
};

template <typename ID, typename Data> struct DataPool : public DataPoolBase<ID, Data> {};
template <typename Data> struct DataPool<NodeID, Data> : public DataPoolBase<NodeID, Data> {
    std::vector<NodeID> roots;

    constexpr auto clear() noexcept -> void {
        roots.clear();
        DataPoolBase<NodeID, Data>::clear();
    }
};

// A collection of multiple AST tree structures
class AST {
  public:
    MAKE_UNALIASED_ITERATOR(std::vector<NodeID>, nodes_.roots)

  public:
    constexpr auto add_root(NodeID id) -> void { nodes_.roots.push_back(id); }
    // Returns the node root ID at the requested index
    [[nodiscard]] constexpr auto operator[](u64 idx) const noexcept -> NodeID {
        return nodes_.roots[idx];
    }

    [[nodiscard]] constexpr auto total_nodes() const noexcept -> usize {
        return nodes_.pool.size();
    }

    template <traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> NodeID {
        constexpr auto kind  = traits::NodeKindOf<Data>::value();
        const auto     index = nodes_.emplace_back(start_token, std::forward<Data>(data));
        return NodeID{kind, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(NodeID id) const noexcept -> const SourceLocation& {
        return nodes_.location_of(id);
    }

    // Returns the node data at the provided node id
    [[nodiscard]] constexpr auto operator[](NodeID id) const noexcept -> auto& {
        return nodes_[id];
    }

    // Returns the casted node data at the requested index
    template <traits::ASTNode N>
    [[nodiscard]] constexpr auto get_as(NodeID id) const noexcept -> const N& {
        ASSERT(id.is<N>(), "Illegal node data retrieval");
        return std::get<N>(nodes_[id]);
    }

    // Returns the casted node data at the requested index if present
    template <traits::ASTNode N>
    [[nodiscard]] constexpr auto get_as_opt(NodeID id) const noexcept -> opt::Option<const N&> {
        if (!id.is<N>()) { return opt::none; }
        return opt::Option<const N&>{std::get<N>(nodes_[id])};
    }

    template <traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, TypeModifier mod, Data&& data) -> ExplicitTypeID {
        constexpr auto kind  = traits::ExplicitTypeKindOf<Data>::value();
        const auto     index = explicit_types_.emplace_back(start_token, std::forward<Data>(data));
        return ExplicitTypeID{kind, mod, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(ExplicitTypeID id) const noexcept
        -> const SourceLocation& {
        return explicit_types_.location_of(id);
    }

    // Returns the type data at the provided type id
    [[nodiscard]] constexpr auto operator[](ExplicitTypeID id) const noexcept -> auto& {
        return explicit_types_[id];
    }

    constexpr auto clear() noexcept -> void {
        nodes_.clear();
        explicit_types_.clear();
    }

  private:
    DataPool<NodeID, NodeData>         nodes_;
    DataPool<ExplicitTypeID, TypeData> explicit_types_;
};

} // namespace ast

} // namespace porpoise

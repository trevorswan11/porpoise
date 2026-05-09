#pragma once

#include <vector>

#include "ast/id.hh"
#include "ast/nodes.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "iterator.hh"

namespace porpoise {

namespace sema { class Type; } // namespace sema

namespace ast {

template <typename IDType, typename DataType> struct DataArrays {
    std::vector<IDType>                       roots;
    std::vector<DataType>                     pool;
    std::vector<SourceLocation>               locations;
    std::vector<opt::detail::Ref<sema::Type>> sema_types;

    constexpr auto clear() noexcept -> void {
        roots.clear();
        pool.clear();
        locations.clear();
        sema_types.clear();
    }
};

class AST {
  public:
    MAKE_UNALIASED_ITERATOR(std::vector<NodeID>, nodes_.roots)

  public:
    constexpr auto add_root(NodeID id) -> void { nodes_.roots.push_back(id); }

    template <traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> NodeID {
        constexpr auto kind  = traits::NodeKindOf<Data>::value();
        const auto     index = static_cast<u64>(nodes_.pool.size());

        nodes_.pool.emplace_back(std::forward<Data>(data));
        nodes_.locations.emplace_back(
            ::porpoise::traits::SourceInfo<syntax::Token>::get(start_token));
        nodes_.sema_types.emplace_back(nullptr);
        return NodeID{kind, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(NodeID id) const noexcept -> const SourceLocation& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return nodes_.locations[id.get_index()];
    }

    [[nodiscard]] constexpr auto operator[](NodeID id) const noexcept -> const NodeData& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return nodes_.pool[id.get_index()];
    }

    [[nodiscard]] constexpr auto has_sema_type(NodeID id) const noexcept -> bool {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return nodes_.sema_types[id.get_index()].has_value();
    }

    [[nodiscard]] constexpr auto get_sema_type(NodeID id) const noexcept -> const sema::Type& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return *nodes_.sema_types[id.get_index()];
    }

    constexpr auto set_sema_type(NodeID id, sema::Type& type) noexcept -> void {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        nodes_.sema_types[id.get_index()].emplace(type);
    }

    template <traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, TypeModifier mod, Data&& data) -> ExplicitTypeID {
        constexpr auto kind  = traits::ExplicitTypeKindOf<Data>::value();
        const auto     index = static_cast<u64>(explicit_types_.pool.size());

        explicit_types_.pool.emplace_back(std::forward<Data>(data));
        explicit_types_.locations.emplace_back(
            ::porpoise::traits::SourceInfo<syntax::Token>::get(start_token));
        explicit_types_.sema_types.emplace_back(nullptr);
        return ExplicitTypeID{kind, mod, start_token.type, index};
    }

    [[nodiscard]] constexpr auto location_of(ExplicitTypeID id) const noexcept
        -> const SourceLocation& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return explicit_types_.locations[id.get_index()];
    }

    // Get the type data at the provided ID
    [[nodiscard]] constexpr auto operator[](ExplicitTypeID id) const noexcept -> const TypeData& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return explicit_types_.pool[id.get_index()];
    }

    constexpr auto clear() noexcept -> void {
        nodes_.clear();
        explicit_types_.clear();
    }

  private:
    DataArrays<NodeID, NodeData>         nodes_;
    DataArrays<ExplicitTypeID, TypeData> explicit_types_;
};

} // namespace ast

} // namespace porpoise

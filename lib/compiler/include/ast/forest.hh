#pragma once

#include <vector>

#include "ast/visitor.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "iterator.hh"

namespace porpoise {

namespace sema { class Type; } // namespace sema

namespace ast {

template <typename ID, typename Data> struct DataPoolBase {
    std::vector<Data>                     pool;
    std::vector<SourceLocation>           locations;
    std::vector<opt::Option<sema::Type&>> sema_types;

    constexpr auto clear() noexcept -> void {
        pool.clear();
        locations.clear();
        sema_types.clear();
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

    // Resizes the semantic type buffer to fit all potential ids
    constexpr auto prepare_sema_types() -> void { sema_types.resize(pool.size()); }

    [[nodiscard]] constexpr auto has_sema_type(ID id) const noexcept -> bool {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return sema_types[id.get_index()].has_value();
    }

    [[nodiscard]] constexpr auto get_sema_type(ID id) const noexcept -> const sema::Type& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return *sema_types[id.get_index()];
    }

    constexpr auto set_sema_type(ID id, sema::Type& type) noexcept -> void {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        sema_types[id.get_index()].emplace(type);
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

#define MAKE_FOREST_API(member, IDType)                                                           \
    [[nodiscard]] constexpr auto location_of(IDType id) const noexcept -> const SourceLocation& { \
        return member.location_of(id);                                                            \
    }                                                                                             \
                                                                                                  \
    [[nodiscard]] constexpr auto operator[](IDType id) const noexcept -> auto& {                  \
        return member[id];                                                                        \
    }                                                                                             \
                                                                                                  \
    [[nodiscard]] constexpr auto has_sema_type(IDType id) const noexcept -> bool {                \
        return member.has_sema_type(id);                                                          \
    }                                                                                             \
                                                                                                  \
    [[nodiscard]] constexpr auto get_sema_type(IDType id) const noexcept -> const sema::Type& {   \
        return member.get_sema_type(id);                                                          \
    }                                                                                             \
                                                                                                  \
    constexpr auto set_sema_type(IDType id, sema::Type& type) noexcept -> void {                  \
        return member.set_sema_type(id, type);                                                    \
    }

// A collection of multiple AST tree structures
class Forest {
  public:
    MAKE_UNALIASED_ITERATOR(std::vector<NodeID>, nodes_.roots)

  public:
    constexpr auto add_root(NodeID id) -> void { nodes_.roots.push_back(id); }

    template <traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> NodeID {
        constexpr auto kind  = traits::NodeKindOf<Data>::value();
        const auto     index = nodes_.emplace_back(start_token, std::forward<Data>(data));
        return NodeID{kind, start_token.type, index};
    }

    MAKE_FOREST_API(nodes_, NodeID)

    template <NodeKind... Kinds>
    constexpr auto set_sema_type(const Handle<Kinds...> id, sema::Type& type) noexcept -> void {
        set_sema_type(*id, type);
    }

    template <traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, TypeModifier mod, Data&& data) -> ExplicitTypeID {
        constexpr auto kind  = traits::ExplicitTypeKindOf<Data>::value();
        const auto     index = explicit_types_.emplace_back(start_token, std::forward<Data>(data));
        return ExplicitTypeID{kind, mod, start_token.type, index};
    }

    MAKE_FOREST_API(explicit_types_, ExplicitTypeID)

    // Resizes semantic type buffers to fit all potential node and type ids
    constexpr auto prepare_sema_types() -> void {
        nodes_.prepare_sema_types();
        explicit_types_.prepare_sema_types();
    }

    constexpr auto clear() noexcept -> void {
        nodes_.clear();
        explicit_types_.clear();
    }

  private:
    DataPool<NodeID, NodeData>         nodes_;
    DataPool<ExplicitTypeID, TypeData> explicit_types_;
};

#undef MAKE_FOREST_API

} // namespace ast

} // namespace porpoise

#pragma once

#include <type_traits>

#include "ast/handle.hh"

namespace porpoise::ast::traits {

template <typename T> struct is_node_id : std::false_type {};
template <> struct is_node_id<NodeID> : std::true_type {};
template <typename T> constexpr auto is_node_id_v = is_node_id<T>::value;

template <typename T> struct is_node_handle : std::false_type {};
template <NodeKind... Kinds> struct is_node_handle<Handle<Kinds...>> : std::true_type {};
template <typename T> constexpr auto is_node_handle_v = is_node_handle<T>::value;

// Represents either a handle or an id that can be used as an index
template <typename T>
concept IndexableNodeID = is_node_id_v<T> || is_node_handle_v<T>;

template <typename T> struct is_explicit_type_id : std::false_type {};
template <> struct is_explicit_type_id<ExplicitTypeID> : std::true_type {};
template <typename T> constexpr auto is_explicit_type_id_v = is_explicit_type_id<T>::value;

// An ID that is not hidden under a Handle or other layer of abstraction
template <typename T>
concept IndexableRawID = is_node_id_v<T> || is_explicit_type_id_v<T>;

// Represents a type id that can be used as an index
template <typename T>
concept IndexableTypeID = is_explicit_type_id_v<T>;

// A generically indexable ID
template <typename T>
concept IndexableID = IndexableTypeID<T> || IndexableNodeID<T>;

} // namespace porpoise::ast::traits

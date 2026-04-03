#pragma once

#include <memory>
#include <utility>

namespace porpoise::mem {

template <typename T, typename D = std::default_delete<T>> using Box = std::unique_ptr<T, D>;
template <typename T, typename... Args> constexpr auto make_box(Args&&... args) -> Box<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Makes a new box from an existing pointer
template <typename T, typename P> constexpr auto box_from(P* ptr) -> Box<T> {
    return std::unique_ptr<T>(static_cast<T*>(ptr));
}

// Makes a new box from an existing box, changing the type as requested
template <typename T, typename P> constexpr auto box_into(Box<P>&& ptr) -> Box<T> {
    return std::unique_ptr<T>(static_cast<T*>(ptr.release()));
}

template <typename T> struct is_box : std::false_type {};
template <typename T, typename D> struct is_box<Box<T, D>> : std::true_type {};
template <typename T> constexpr bool is_box_v = is_box<T>::value;

// If you find yourself using this, think really hard about the decisions that led you here...
template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace porpoise::mem

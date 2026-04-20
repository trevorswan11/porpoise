#pragma once

#include <type_traits>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include <fmt/format.h>

#include "option.hpp"
#include "source_loc.hpp"
#include "types.hpp"

namespace porpoise {

namespace detail {

[[nodiscard]] auto format_diagnostic(const opt::Option<std::string>&    message,
                                     std::string_view                   error_name,
                                     const opt::Option<SourceLocation>& location) -> std::string;

} // namespace detail

template <typename E>
    requires std::is_scoped_enum_v<E>
class Diagnostic {
  public:
    explicit Diagnostic(E err) noexcept : error_{err} {}
    Diagnostic(E err, usize line, usize column) noexcept : error_{err}, loc_{{line, column}} {}
    Diagnostic(std::string msg, E err, usize line, usize column) noexcept
        : message_{std::move(msg)}, error_{err}, loc_{{line, column}} {}

    template <Locateable T>
    Diagnostic(std::string msg, E err, const T& t) noexcept
        : message_{std::move(msg)}, error_{err}, loc_{SourceInfo<T>::get(t)} {}

    template <Locateable T> Diagnostic(E err, T t) : error_{err}, loc_{SourceInfo<T>::get(t)} {}

    Diagnostic(Diagnostic& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, loc_{std::move(other.loc_)} {}

    auto has_msg() const noexcept -> bool { return message_.has_value(); }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    [[nodiscard]] auto to_string() const noexcept -> std::string {
        return detail::format_diagnostic(message_, magic_enum::enum_name(error_), loc_);
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

  private:
    opt::Option<std::string>    message_{};
    E                           error_;
    opt::Option<SourceLocation> loc_{};
};

} // namespace porpoise

template <typename E> struct fmt::formatter<porpoise::Diagnostic<E>> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::Diagnostic<E>& d, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};

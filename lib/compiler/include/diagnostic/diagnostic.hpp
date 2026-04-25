#pragma once

#include <utility>

#include <magic_enum/magic_enum.hpp>

#include <fmt/format.h>

#include "module/source_location.hpp"

#include "enum.hpp"
#include "option.hpp"
#include "types.hpp"

namespace porpoise {

namespace detail {

[[nodiscard]] auto format_diagnostic(const opt::Option<std::string>&    message,
                                     std::string_view                   error_name,
                                     const opt::Option<std::string>&    source_path,
                                     const opt::Option<SourceLocation>& location) -> std::string;

} // namespace detail

template <ScopedEnum E> class Diagnostic {
  public:
    explicit Diagnostic(E err) noexcept : error_{err} {}
    Diagnostic(E err, usize line, usize column) noexcept : error_{err}, loc_{{line, column}} {}
    Diagnostic(std::string msg, E err, usize line, usize column) noexcept
        : message_{std::move(msg)}, error_{err}, loc_{{line, column}} {}
    Diagnostic(std::string msg, E err) noexcept : message_{std::move(msg)}, error_{err} {}

    template <Locateable T>
    Diagnostic(std::string msg, E err, const T& t) noexcept
        : message_{std::move(msg)}, error_{err}, loc_{SourceInfo<T>::get(t)} {}

    template <Locateable T> Diagnostic(E err, T t) : error_{err}, loc_{SourceInfo<T>::get(t)} {}

    Diagnostic(Diagnostic& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, loc_{std::move(other.loc_)} {}

    auto has_msg() const noexcept -> bool { return message_.has_value(); }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    [[nodiscard]] auto to_string(const opt::Option<std::string>& source_path = opt::none) const
        -> std::string {
        return detail::format_diagnostic(
            message_, magic_enum::enum_name(error_), source_path, loc_);
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

  private:
    opt::Option<std::string>    message_{};
    E                           error_;
    opt::Option<SourceLocation> loc_{};
};

template <typename T> struct is_diagnostic : std::false_type {};
template <typename T> struct is_diagnostic<Diagnostic<T>> : std::true_type {};
template <typename T> constexpr bool is_diagnostic_v = is_diagnostic<T>::value;

template <typename T>
concept DiagnosticType = is_diagnostic_v<T>;

} // namespace porpoise

template <typename E> struct fmt::formatter<porpoise::Diagnostic<E>> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::Diagnostic<E>& d, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};

#pragma once

#include <ostream>
#include <sstream>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include <fmt/format.h>

#include "diagnostic/source_location.hpp"

#include "enum.hpp"
#include "option.hpp"
#include "types.hpp"

namespace porpoise {

namespace detail {

// A decomposed diagnostic that contains all information for base formatting
struct FormattableDiagnostic {
    const opt::Option<std::string>&    message;
    std::string_view                   error_name;
    const opt::Option<SourceLocation>& location;
};

auto format_diagnostic(std::ostream&                   os,
                       FormattableDiagnostic&&         diag,
                       const opt::Option<std::string>& source_path) -> std::ostream&;

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

    // Moves the passed diagnostic into a new one with a error code
    Diagnostic(Diagnostic&& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, loc_{std::move(other.loc_)} {}

    // Moves the passed diagnostic into a new one with a specified source location
    template <Locateable T>
    Diagnostic(Diagnostic&& other, const T& t) noexcept
        : message_{std::move(other.message_)}, error_{other.error_}, loc_{SourceInfo<T>::get(t)} {}

    [[nodiscard]] auto to_string(const opt::Option<std::string>& source_path = opt::none) const
        -> std::string {
        std::stringstream ss;
        detail::format_diagnostic(ss, to_formattable(), source_path);
        return ss.str();
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

    [[nodiscard]] auto to_formattable() const noexcept -> detail::FormattableDiagnostic {
        return {message_, magic_enum::enum_name(error_), loc_};
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

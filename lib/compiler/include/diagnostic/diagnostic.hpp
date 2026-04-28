#pragma once

#include <ostream>
#include <sstream>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include <fmt/color.h>
#include <fmt/format.h>

#include "diagnostic/source_location.hpp"

#include "enum.hpp"
#include "option.hpp"
#include "types.hpp"

namespace porpoise {

enum class DiagnosticLevel : u8 {
    ERROR,
    WARNING,
};

namespace detail {

namespace style {

constexpr auto BASE    = fmt::text_style{};
constexpr auto SOURCE  = fmt::fg(fmt::color::white) | fmt::emphasis::bold;
constexpr auto ERROR   = fmt::fg(fmt::color::red);
constexpr auto WARNING = fmt::fg(fmt::color::light_yellow);
constexpr auto CARET   = fmt::fg(fmt::color::green);

} // namespace style

// Returns the level fit for diagnostic printing
[[nodiscard]] constexpr auto level_name(DiagnosticLevel level) noexcept -> std::string_view {
    switch (level) {
    case DiagnosticLevel::ERROR:   return "error";
    case DiagnosticLevel::WARNING: return "warning";
    default:                       return "";
    }
}

// Returns the level's style for diagnostic printing
[[nodiscard]] constexpr auto level_style(DiagnosticLevel level) noexcept {
    switch (level) {
    case DiagnosticLevel::ERROR:   return style::ERROR;
    case DiagnosticLevel::WARNING: return style::WARNING;
    default:                       return style::BASE;
    }
}

// A decomposed diagnostic that contains all information for base formatting
struct FormattableDiagnostic {
    const opt::Option<std::string>&     message;
    const opt::Option<SourceLocation>&  location;
    const opt::Option<DiagnosticLevel>& level;
};

auto format_diagnostic(std::ostream&                   os,
                       FormattableDiagnostic&&         diag,
                       const opt::Option<std::string>& source_path,
                       opt::Option<bool>               in_terminal) -> std::ostream&;

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

    [[nodiscard]] auto to_string(const opt::Option<std::string>& source_path = opt::none,
                                 opt::Option<bool> in_terminal = opt::none) const -> std::string {
        std::stringstream ss;
        detail::format_diagnostic(ss, to_formattable(), source_path, in_terminal);
        return ss.str();
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && loc_ == other.loc_;
    }

    [[nodiscard]] auto to_formattable() const noexcept -> detail::FormattableDiagnostic {
        return {message_, loc_, level_};
    }

    // Diagnostics are always ERROR by default, see `unset_level`
    auto set_level(DiagnosticLevel level) noexcept -> void { level_.emplace(level); }
    auto unset_level() noexcept -> void { level_.reset(); }

  private:
    opt::Option<std::string>     message_{};
    E                            error_;
    opt::Option<SourceLocation>  loc_{};
    opt::Option<DiagnosticLevel> level_{DiagnosticLevel::ERROR};
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

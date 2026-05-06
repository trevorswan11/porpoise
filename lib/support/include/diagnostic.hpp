#pragma once

#include <ostream>
#include <span>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include <fmt/color.h>
#include <fmt/format.h>

#include "iterator.hpp"
#include "option.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace porpoise {

// Should be zero indexed and only 1-indexed at print time
struct SourceLocation {
    usize line   = 0;
    usize column = 0;

    SourceLocation() noexcept = default;
    SourceLocation(usize line, usize column) noexcept : line{line}, column{column} {}

    auto operator==(const SourceLocation& other) const noexcept -> bool {
        return line == other.line && column == other.column;
    }
};

template <typename T> struct SourceInfo;

template <typename T>
concept Locateable = requires(T t) {
    { SourceInfo<T>::get(t) } -> std::same_as<SourceLocation>;
};

template <> struct SourceInfo<std::pair<usize, usize>> {
    static auto get(const std::pair<usize, usize>& p) noexcept -> SourceLocation {
        return {p.first, p.second};
    }
};

namespace mod { struct Module; } // namespace mod

enum class DiagnosticLevel : u8 {
    ERROR,
    WARNING,
};

namespace style {

constexpr auto BASE    = fmt::text_style{};
constexpr auto SOURCE  = fmt::fg(fmt::color::white) | fmt::emphasis::bold;
constexpr auto ERROR   = fmt::fg(fmt::color::red);
constexpr auto WARNING = fmt::fg(fmt::color::light_yellow);
constexpr auto CARET   = fmt::fg(fmt::color::green);

} // namespace style

namespace detail {

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
    const opt::Option<std::string>&    message;
    const opt::Option<SourceLocation>& location;
    std::string_view                   error_name;
    const opt::Enum<DiagnosticLevel>&  level;
};

auto format_diagnostic(std::ostream&                   os,
                       FormattableDiagnostic&&         diag,
                       const opt::Option<std::string>& source_path,
                       opt::Option<bool>               in_terminal) -> std::ostream&;

} // namespace detail

template <ScopedEnum E> class Diagnostic {
  public:
    explicit Diagnostic(E err) noexcept : error_{err} {}
    Diagnostic(E err, usize line, usize column) noexcept : loc_{{line, column}}, error_{err} {}
    Diagnostic(opt::Option<std::string> msg, E err, usize line, usize column) noexcept
        : message_{std::move(msg)}, loc_{{line, column}}, error_{err} {}
    Diagnostic(opt::Option<std::string> msg, E err) noexcept
        : message_{std::move(msg)}, error_{err} {}

    template <Locateable T>
    Diagnostic(opt::Option<std::string> msg, E err, const T& t) noexcept
        : message_{std::move(msg)}, loc_{SourceInfo<T>::get(t)}, error_{err} {}

    template <Locateable T> Diagnostic(E err, T t) : loc_{SourceInfo<T>::get(t)}, error_{err} {}

    // Moves the passed diagnostic into a new one with an error code
    Diagnostic(Diagnostic&& other, E err) noexcept
        : message_{std::move(other.message_)}, loc_{std::move(other.loc_)}, error_{err} {}

    // Moves the passed diagnostic into a new one with a specified source location
    template <Locateable T>
    Diagnostic(Diagnostic&& other, const T& t) noexcept
        : message_{std::move(other.message_)}, loc_{SourceInfo<T>::get(t)}, error_{other.error_} {}

    [[nodiscard]] auto to_string(const opt::Option<std::string>& source_path = opt::none,
                                 opt::Option<bool> in_terminal = opt::none) const -> std::string {
        std::stringstream ss;
        detail::format_diagnostic(ss, to_formattable(), source_path, in_terminal);
        return ss.str();
    }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && loc_ == other.loc_ && error_ == other.error_ &&
               level_ == other.level_;
    }

    MAKE_GETTER(message, const opt::Option<std::string>&)
    [[nodiscard]] auto to_formattable() const noexcept -> detail::FormattableDiagnostic {
        return {message_, loc_, magic_enum::enum_name(error_), level_};
    }

    // Diagnostics are always ERROR by default, see `unset_level`
    auto set_level(DiagnosticLevel level) noexcept -> void { level_.emplace(level); }
    auto unset_level() noexcept -> void { level_.reset(); }

  private:
    opt::Option<std::string>    message_{};
    opt::Option<SourceLocation> loc_{};
    E                           error_;
    opt::Enum<DiagnosticLevel>  level_{DiagnosticLevel::ERROR};
};

template <typename T> struct is_diagnostic : std::false_type {};
template <typename T> struct is_diagnostic<Diagnostic<T>> : std::true_type {};
template <typename T> constexpr bool is_diagnostic_v = is_diagnostic<T>::value;

template <typename T>
concept DiagnosticType = is_diagnostic_v<T>;

template <DiagnosticType D> class DiagnosticList {
  public:
    MAKE_ITERATOR(Diagnostics, std::vector<D>, diagnostics_) // cppcheck-suppress syntaxError

  public:
    explicit DiagnosticList(opt::Option<bool> in_terminal = opt::none) noexcept
        : in_terminal_{in_terminal} {}
    ~DiagnosticList() = default;

    MAKE_MOVE_ONLY(DiagnosticList)

    auto push_back(const D& d) -> void { diagnostics_.push_back(d); }

    template <typename... Args> auto emplace_back(Args&&... args) -> void {
        diagnostics_.emplace_back(std::forward<Args>(args)...);
    }

    template <typename Self>
    [[nodiscard]] auto operator[](this Self&& self, usize idx) noexcept -> auto& {
        return self.diagnostics_[idx];
    }

    template <typename Self> [[nodiscard]] auto at(this Self&& self, usize idx) -> auto& {
        return self.diagnostics_.at(idx);
    }

    operator std::span<const D>() const { return diagnostics_; }

    // Creates a new list with the same terminal behavior
    [[nodiscard]] auto create_new() const -> DiagnosticList { return DiagnosticList{in_terminal_}; }
    auto get_terminal_status() const noexcept -> const opt::Option<bool>& { return in_terminal_; }

  private:
    Diagnostics       diagnostics_;
    opt::Option<bool> in_terminal_;
};

} // namespace porpoise

template <> struct fmt::formatter<porpoise::SourceLocation> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::SourceLocation& loc, F& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}", loc.line + 1, loc.column + 1);
    }
};

template <typename E> struct fmt::formatter<porpoise::Diagnostic<E>> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::Diagnostic<E>& d, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};

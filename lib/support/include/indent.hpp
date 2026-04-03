#pragma once

#include <ranges>
#include <string>
#include <vector>

namespace porpoise {

namespace symbols {

constexpr std::string_view VERT_BAR{"\u2502   "};
constexpr std::string_view T_BRANCH{"\u251C\u2500 "};
constexpr std::string_view L_BRANCH{"\u2514\u2500 "};
constexpr std::string_view EMPTY{"    "};

} // namespace symbols

class Indent {
  public:
    class Guard {
      public:
        Guard(Indent& i, bool last) : indent_{i} { indent_.push(last); }
        ~Guard() { indent_.pop(); }

      private:
        Indent& indent_;
    };

  public:
    auto push(bool last) -> void { levels_.push_back(last); }
    auto pop() -> void { levels_.pop_back(); }

    [[nodiscard]] auto current_branch() const -> std::string {
        if (levels_.empty()) { return {}; }
        auto res = levels_ | std::views::take(levels_.size() - 1) |
                   std::views::transform(
                       [](auto level) { return level ? symbols::EMPTY : symbols::VERT_BAR; }) |
                   std::views::join | std::ranges::to<std::string>();
        res += levels_.back() ? symbols::L_BRANCH : symbols::T_BRANCH;
        return res;
    }

  private:
    std::vector<bool> levels_;
};

} // namespace porpoise

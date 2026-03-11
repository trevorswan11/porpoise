#include "indent.hpp"

namespace conch {

auto Indent::current_branch() const -> std::string {
    if (levels_.empty()) { return ""; }
    std::string res = prefix_only();
    res += (levels_.back() ? symbols::L_BRANCH : symbols::T_BRANCH);
    return res;
}

auto Indent::prefix_only() const -> std::string {
    std::string res;
    for (size_t i = 0; i + 1 < levels_.size(); ++i) {
        res += (levels_[i] ? symbols::EMPTY : symbols::VERT_BAR);
    }
    return res;
}

} // namespace conch

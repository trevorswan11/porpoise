#include "diagnostic/source_file.hpp"

#include "string.hpp"

namespace porpoise {

LineOffsets::LineOffsets(std::string_view input) {
    offsets_.emplace_back(0);
    for (usize i = 0; i < input.size(); ++i) {
        if (input[i] == '\n') { offsets_.emplace_back(i + 1); }
    }
}

auto SourceFile::get_diagnostic_strings(const SourceLocation& loc) const
    -> std::pair<std::string_view, opt::Option<std::string>> {
    if (loc.line > offsets_.size()) { return {"<invalid line>", opt::none}; }

    const auto start  = offsets_[loc.line];
    const auto end    = loc.line < offsets_.size() ? offsets_[loc.line + 1] : offsets_.size();
    auto       substr = string::substr(source_, start, end - start);

    // Count skipped on the left but not right since the caret is right-clipped
    usize skipped = 0;
    substr        = string::trim_left(substr, skipped);
    substr        = string::trim_right(substr);

    // Adjust the column number based on skipped spaces
    if (loc.column < skipped) { return {substr, opt::none}; }
    const auto true_col = loc.column - skipped;
    if (true_col + 1 > substr.size()) { return {substr, opt::none}; }

    // The caret gets put one after the column size since the location in 0-indexed
    std::string caret_line;
    caret_line.reserve(true_col + 1);
    for (usize i = 0; i < true_col; ++i) {
        if (substr[i] == '\t') {
            caret_line += '\t';
        } else {
            caret_line += ' ';
        }
    }
    caret_line += '^';

    return {substr, std::move(caret_line)};
}

} // namespace porpoise

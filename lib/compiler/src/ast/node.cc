#include "ast/node.hh"

#include "enum.hh"

namespace porpoise::ast {

namespace {

constexpr auto NODE_NAMES = [] {
    EnumMap<NodeKind, std::string_view> names{"expression"};

    names[NodeKind::ENUM_EXPRESSION]     = "enum";
    names[NodeKind::FUNCTION_EXPRESSION] = "function";
    names[NodeKind::UNION_EXPRESSION]    = "union";
    names[NodeKind::STRUCT_EXPRESSION]   = "struct";

    for (const auto kind : enum_range<NodeKind::BLOCK_STATEMENT, NodeKind::USING_STATEMENT>()) {
        names[kind] = "statement";
    }
    return names;
}();

} // namespace

auto Node::display_name() const noexcept -> std::string_view { return NODE_NAMES[kind_]; }

} // namespace porpoise::ast

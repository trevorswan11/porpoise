#include <algorithm>
#include <type_traits>

#include "ast/statements/members.hh"

#include "ast/expressions/function.hh"
#include "ast/visitor.hh"

namespace porpoise::ast {

auto Members::accept(const Member& member, Visitor& v) -> void {
    return std::visit([&](const auto& underlying) { v.visit(*underlying); }, member);
}

namespace {

// Returns an actual value only if a terminal condition was found
[[nodiscard]] auto validate_common(const DeclStatement& decl) noexcept -> opt::Option<bool> {
    // Members that violate this wouldn't be usable with C
    if (decl.has_modifier(DeclModifiers::EXTERN) || decl.has_modifier(DeclModifiers::EXPORT)) {
        return false;
    }

    if (decl.get_value().is<FunctionExpression>()) { return true; }
    return opt::none;
}

} // namespace

auto Members::validate_struct_decl(const DeclStatement& decl) noexcept -> bool {
    if (const auto result = validate_common(decl)) { return *result; }

    // A non-static member must always be a variable to simplify the mental model
    if (!decl.has_modifier(DeclModifiers::STATIC)) {
        return decl.has_modifier(DeclModifiers::VARIABLE);
    }
    return true;
}

auto Members::validate_non_struct_decl(const DeclStatement& decl) noexcept -> bool {
    if (const auto result = validate_common(decl)) { return *result; }
    return decl.has_modifier(DeclModifiers::STATIC);
}

auto Members::is_equal(const Members& other) const noexcept -> bool {
    return std::ranges::equal(
        members_, other.members_, [](const Member& member_a, const Member& member_b) {
            if (member_a.index() != member_b.index()) { return false; }
            return std::visit(
                [&](const auto& node) {
                    return *node == *std::get<std::remove_cvref_t<decltype(node)>>(member_b);
                },
                member_a);
        });
}

} // namespace porpoise::ast

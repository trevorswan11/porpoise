#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/sema.hh"
#include "sema/symbol.hh"

namespace porpoise::tests {

namespace syms = sema::symbols;

TEST_CASE("Array resolution") {
    auto [ctx, idx] = helpers::resolve_and_check("const a: [2]i32 = [2]i32{1, 2, };");
    [[maybe_unused]] const auto [msg_sym, msg_sym_data, msg_node_data, msg_type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("a", idx);
}

} // namespace porpoise::tests

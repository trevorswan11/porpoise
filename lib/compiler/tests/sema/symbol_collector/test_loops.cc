#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/type.hh"

#include "types.hh"

namespace porpoise::tests {

namespace mut = sema::types::mut;

namespace {

[[nodiscard]] auto test_loop(std::string_view input, usize expected_reg_count, usize loop_block_idx)
    -> helpers::SemaTestContext {
    auto [ctx, idx] = helpers::collect_and_check(input);

    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == expected_reg_count);
    const auto& symbol_a = registry.get_from(0, "a");
    CHECK_FALSE(symbol_a.has_kind());

    const auto symbolic_node = symbol_a.as_opt<sema::symbols::Node>();
    REQUIRE(symbolic_node);
    const auto& decl = ctx.root_mod->ast.get_as<ast::DeclStatement>(*symbolic_node);

    auto& pool = ctx.analyzer.get_pool();
    REQUIRE(ctx.root_mod->has_sema_type(**decl.value));
    auto&       expected_type = pool[{sema::TypeKind::BLOCK, mut::CONSTANT, loop_block_idx}];
    const auto& actual_type   = ctx.root_mod->get_sema_type(**decl.value);
    CHECK(&expected_type == &actual_type);
    return std::move(ctx);
}

} // namespace

TEST_CASE("Do-while loop collection") {
    auto ctx =
        test_loop("const a := do { const foo := bar; } while (blk: { const foo := bar; });", 4, 1);
    ctx.test_common_decl_collection(1);
    ctx.test_common_decl_collection(3);
}

TEST_CASE("For loop collection") {
    auto ctx = test_loop("const a := for (0..5, blk: { const foo := bar; }) |i, j| { const foo := "
                         "bar; } else { const foo := bar; };",
                         5,
                         3);

    const auto& loop_table = ctx.analyzer.get_table(3);
    REQUIRE(loop_table.has("i"));
    CHECK(loop_table.get("i").as_opt<sema::symbols::ForLoopCapture>());
    REQUIRE(loop_table.has("j"));
    CHECK(loop_table.get("j").as_opt<sema::symbols::ForLoopCapture>());

    ctx.test_common_decl_collection(2);
    ctx.test_common_decl_collection(3);
    ctx.test_common_decl_collection(4);
}

TEST_CASE("Infinite loop collection") {
    auto ctx = test_loop("const a := loop { const foo := bar; };", 2, 1);
    ctx.test_common_decl_collection(1);
}

TEST_CASE("While loop collection") {
    auto ctx = test_loop("const a := while (blk: { const foo := bar; }) : (i += blk: { const foo "
                         ":= bar; }) { const foo := bar; } else { const foo := bar; };",
                         7,
                         5);
    ctx.test_common_decl_collection(2);
    ctx.test_common_decl_collection(4);
    ctx.test_common_decl_collection(5);
    ctx.test_common_decl_collection(6);
}

TEST_CASE("Well-placed loop control flow") {
    SECTION("For loops") {
        helpers::collect_and_check("const a := for (0..5) |i| { break; };");
        helpers::collect_and_check("const a := for (0..5) |i| { continue; };");
    }

    SECTION("Do-while loop") {
        helpers::collect_and_check("const a := do { break; } while (true);");
        helpers::collect_and_check("const a := do { continue; } while (true);");
    }

    SECTION("Infinite loop") {
        helpers::collect_and_check("const a := loop { break; };");
        helpers::collect_and_check("const a := loop { continue; };");
    }

    SECTION("While loops") {
        helpers::collect_and_check("const a := while (true) { break; };");
        helpers::collect_and_check("const a := while (true) { continue; };");
    }
}

TEST_CASE("Non-break collected as separate scope") {
    helpers::collect_and_check(
        "const a := for (0..5) |i| { const foo := bar; } else { const foo := bar; };");

    helpers::collect_and_check(
        "const a := while (true) : (i += 1) { const foo := bar; } else { const foo := bar; };");
}

TEST_CASE("Non-break collection shadowing") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 55uz}});

    helpers::test_collector_fail(
        "const a := while (true) : (i += 1) { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 64uz}});
}

TEST_CASE("Shadowing in loops") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 28uz}});

    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var i: i32; };",
        sema::Diagnostic{"Redeclaration of symbol 'i'. Previous declaration here: 1:24",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 28uz}});

    helpers::test_collector_fail(
        "const a := loop { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 18uz}});

    helpers::test_collector_fail(
        "const a := while (true) { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 26uz}});
}

} // namespace porpoise::tests

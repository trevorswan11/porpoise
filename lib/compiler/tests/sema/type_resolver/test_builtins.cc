#include <array>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"

namespace porpoise::tests {

namespace syms = sema::symbols;

namespace {

// Checks for the builtin's presence in the prelude table and its return type when called
auto test_builtin_resolve(const syntax::Builtin& builtin,
                          std::string_view       mock_params,
                          auto&&                 expected_type_fn,
                          std::string_view       prelude = "") -> void {
    auto [ctx, idx] = helpers::resolve_and_check(
        fmt::format("{}const foo := {}({});", prelude, builtin.name, mock_params));

    // Quickly check to make sure the prelude table has the builtin
    const auto [builtin_sym, builtin_sym_data] = ctx->get_symbol<syms::Builtin>(
        builtin.name, helpers::unwrap(ctx->analyzer.get_prelude_index_opt()));
    CHECK(builtin_sym.get_kind_opt() == sema::SymbolKind::CALLABLE);
    const auto& builtin_type = builtin_sym_data.get_type();
    CHECK(builtin_type.as_opt<sema::types::BuiltinFunction>());

    // Now validate the actual declaration and call
    const sema::Type& expected_type = expected_type_fn(*ctx);
    const auto [decl_sym, decl_sym_data, decl_node_data, decl_type] =
        ctx->get_ast_type_sym_info<syms::Node, ast::DeclStatement>("foo", idx);
    CHECK(&expected_type == &decl_type);

    const auto& call =
        helpers::unwrap(ctx->root_mod->ast.get_as_opt<ast::CallExpression>(*decl_node_data.value));
    const auto& call_type = helpers::unwrap(ctx->root_mod->get_sema_type_opt(call.function));
    CHECK(&builtin_type == &call_type);
}

} // namespace

namespace bis = syntax::builtins;

TEST_CASE("Builtin 'safe' casts") {
    constexpr std::array safe_casts{bis::ALIGN_CAST, bis::PTR_CAST, bis::BIT_CAST, bis::AS};
    for (const auto& bi : safe_casts) {
        test_builtin_resolve(bi, "i32, 23uz", [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::I32);
        });
    }
}

TEST_CASE("Builtin 'unsafe' casts") {
    test_builtin_resolve(bis::CONST_CAST, "^23", [](helpers::SemaTestContext& ctx) -> sema::Type& {
        return ctx.get_type<sema::types::mut::MUTABLE>(sema::TypeKind::POINTER,
                                                       ctx.get_type(sema::TypeKind::I32));
    });

    test_builtin_resolve(
        bis::VOLATILE_CAST,
        "v",
        [](helpers::SemaTestContext& ctx) -> sema::Type& {
            return ctx.get_type(sema::TypeKind::I32);
        },
        "const v: volatile i32 = 23;");
}

TEST_CASE("Builtin bit/byte operations") {
#if 0
    test_builtin_resolve(bis::ALIGN_OF, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::SIZE_OF, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::CLZ, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::CTZ, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::POP_COUNT, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtin type introspection") {
#if 0
    test_builtin_resolve(bis::TYPE_OF, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::TAG_NAME, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtin pointer conversions") {
#if 0
    test_builtin_resolve(bis::INT_FROM_PTR, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::PTR_FROM_ARRAY, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::PTR_FROM_INT, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::SLICE_FROM_PTR, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtins memory operation") {
#if 0
    test_builtin_resolve(bis::MEMCPY, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::MEMSET, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::MEMMOVE, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtin arithmetic") {
#if 0
    test_builtin_resolve(bis::MUL_ADD, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::SQRT, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::SIN, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::COS, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::TAN, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::EXP, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::EXP2, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::LOG, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::LOG2, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::LOG10, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::ABS, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::FLOOR, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::CEIL, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
    test_builtin_resolve(bis::DIV_MOD, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtin control flow") {
#if 0
    test_builtin_resolve(bis::PANIC, "", [](helpers::SemaTestContext& ctx) -> sema::Type& {});
#endif
}

TEST_CASE("Builtin function arity mismatch") {
    helpers::test_resolver_fail("const foo := @sizeOf();",
                                sema::Diagnostic{"Builtin expects 1 arguments, found 0",
                                                 sema::Error::ARITY_MISMATCH,
                                                 std::pair{0uz, 13uz}});
}

TEST_CASE("Const cast quick type checking") {
    helpers::test_resolver_fail("const foo := @constCast(1);",
                                sema::Diagnostic{"Expected pointer or reference type; found 'i32'",
                                                 sema::Error::TYPE_MISMATCH,
                                                 std::pair{0uz, 24uz}});
}

TEST_CASE("Other builtin quick type mismatch") {
    helpers::test_resolver_fail(
        "const foo := @ptrFromArray(1);",
        sema::Diagnostic{"Expected an array-yielding expression; found 'i32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 27uz}});

    helpers::test_resolver_fail("const foo := @ptrFromInt(i32, 0xdeadbeeful);",
                                sema::Diagnostic{"Expected a pointer type; found 'i32'",
                                                 sema::Error::TYPE_MISMATCH,
                                                 std::pair{0uz, 25uz}});

    helpers::test_resolver_fail(
        "const foo := @sliceFromPtr(1, 20uz);",
        sema::Diagnostic{"Expected a pointer-yielding expression; found 'i32'",
                         sema::Error::TYPE_MISMATCH,
                         std::pair{0uz, 27uz}});
}

} // namespace porpoise::tests

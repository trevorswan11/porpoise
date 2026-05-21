#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <variant>

#include "ast/expression.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "module/module.hh"
#include "sema/symbol.hh"

#include "option.hh"
#include "sema/type.hh"
#include "types.hh"

namespace porpoise::tests {

using MockFile = helpers::MockFile;

constexpr std::string_view main_porp{R"(
import std;

pub const main := fn(args: [][:0]u8): i32 {
    const message := "Hello, world!";
    std::io::println(message);
    return 0;
};
)"};

constexpr std::string_view std_porp{R"(
pub import "io.porp" as io;
)"};

constexpr std::string_view io_porp{R"(
pub const println := fn(str: []u8): void {};
)"};

namespace {

// The table index should point to the table where the module was first declared
[[nodiscard]] auto check_inner_module(const sema::SymbolTableRegistry& registry,
                                      mod::Module&                     enclosing_module,
                                      usize                            table_idx,
                                      std::string_view                 name) -> mod::Module& {
    const auto& symbol = helpers::unwrap(registry.get_from_opt(table_idx, name));
    CHECK(symbol.get_kind_opt() == sema::SymbolKind::MODULE);
    CHECK(symbol.get_status() == sema::SymbolStatus::RESOLVED);
    const auto node = helpers::unwrap(symbol.as_opt<sema::symbols::Node>());
    CHECK(node.is<ast::ImportStatement>());

    const auto& type        = helpers::unwrap(enclosing_module.get_sema_type_opt(node));
    const auto& module_type = helpers::unwrap(type.as_opt<sema::types::Module>());
    auto&       module      = module_type.imported;
    CHECK(module.is_ok());
    return module;
}

} // namespace

TEST_CASE("Full sema pipeline") {
    auto        ctx      = helpers::analyze("main.porp",
                                main_porp,
                                MockFile{"std.porp", std_porp, "std"},
                                MockFile{"io.porp", io_porp});
    const auto& registry = ctx.analyzer.get_registry();
    auto&       pool     = ctx.analyzer.get_pool();
    REQUIRE(registry.size() == 6);

    auto& root_module = *ctx.root_mod;
    CHECK(root_module.root_table_idx == 0);
    auto& std_module = check_inner_module(registry, root_module, 0, "std");
    CHECK(std_module.root_table_idx == 1);
    auto& io_module = check_inner_module(registry, std_module, 1, "io");
    CHECK(registry.get_from(1, "io").is_public(std_module));
    CHECK(io_module.root_table_idx == 2);
    // TODO: Verify function types

    SECTION("Main function validation") {
        const auto& symbol = helpers::unwrap(registry.get_from_opt(0, "main"));
        CHECK(symbol.is_public(root_module));
        CHECK(symbol.get_kind_opt() == sema::SymbolKind::CALLABLE);
        CHECK(symbol.get_status() == sema::SymbolStatus::RESOLVED);
        const auto  node    = helpers::unwrap(symbol.as_opt<sema::symbols::Node>());
        const auto& fn_decl = helpers::unwrap(root_module.ast.get_as_opt<ast::DeclStatement>(node));
        const auto& fn_expr =
            helpers::unwrap(root_module.ast.get_as_opt<ast::FunctionExpression>(*fn_decl.value));

        const auto& fn_type = helpers::unwrap(root_module.get_sema_type_opt(node));
        CHECK(fn_type.get_symbol_table_idx_opt() == 4);
        const auto  fn_idx = fn_type.get_symbol_table_idx();
        const auto& fn     = helpers::unwrap(fn_type.as_opt<sema::types::Function>());
        CHECK(fn.params.size() == 1);

        // Verify the parameter type
        const auto& u8_type = pool[{sema::TypeKind::U8, sema::types::mut::CONSTANT}];
        const auto& u8_slice_type =
            pool[{sema::TypeKind::SLICE, sema::types::mut::CONSTANT, true, u8_type}];
        const auto& u8_slice_slice_type =
            pool[{sema::TypeKind::SLICE, sema::types::mut::CONSTANT, false, u8_slice_type}];

        // Verify the parameter type & symbol
        {
            const auto& param_type = *fn.params[0];
            CHECK(&param_type == &u8_slice_slice_type);

            const auto& param_symbol = helpers::unwrap(registry.get_from_opt(fn_idx, "args"));
            CHECK(param_symbol.get_kind_opt() == sema::SymbolKind::VALUE);
            CHECK(param_symbol.get_status() == sema::SymbolStatus::RESOLVED);
            const auto args_param =
                helpers::unwrap(param_symbol.as_opt<sema::symbols::Parameter>());
            const auto& args_param_type =
                helpers::unwrap(root_module.get_sema_type_opt(args_param.ident));
            CHECK(&args_param_type == &u8_slice_slice_type);
            const auto& args_param_explicit_type =
                helpers::unwrap(root_module.get_sema_type_opt(args_param.explicit_type));
            CHECK(&args_param_explicit_type == &u8_slice_slice_type);
        }

        // Verify the return type
        {
            const auto& return_type = fn.return_type;
            const auto& i32_type    = pool[{sema::TypeKind::I32, sema::types::mut::CONSTANT}];
            CHECK(&return_type == &i32_type);
        }

        // Verify the function body
        {
            const auto& msg_symbol = helpers::unwrap(registry.get_from_opt(fn_idx, "message"));
            CHECK_FALSE(msg_symbol.is_public(root_module));
            CHECK(msg_symbol.get_kind_opt() == sema::SymbolKind::VALUE);
            CHECK(msg_symbol.get_status() == sema::SymbolStatus::RESOLVED);
            const auto msg_node = helpers::unwrap(msg_symbol.as_opt<sema::symbols::Node>());

            const auto& decl =
                helpers::unwrap(root_module.ast.get_as_opt<ast::DeclStatement>(msg_node));
            const auto& decl_value =
                helpers::unwrap(root_module.ast.get_as_opt<ast::StringExpression>(*decl.value));

            const auto& msg_type = helpers::unwrap(root_module.get_sema_type_opt(msg_node));
            const auto& type     = pool[{sema::TypeKind::ARRAY,
                                         sema::types::mut::CONSTANT,
                                         false,
                                         decl_value.value.size(),
                                         u8_type}];
            CHECK(&msg_type == &type);

            const auto& fn_body =
                helpers::unwrap(root_module.ast.get_as_opt<ast::BlockStatement>(fn_expr.body));
            const auto call_id = [&] {
                for (const auto& body_id : fn_body) {
                    if (const auto& expr_stmt =
                            root_module.ast.get_as_opt<ast::ExpressionStatement>(body_id)) {
                        if (expr_stmt->expression.is<ast::CallExpression>()) {
                            return expr_stmt->expression;
                        }
                    }
                }
                FAIL("Call expression could not be found");
            }();

            const auto& call_expr =
                helpers::unwrap(root_module.ast.get_as_opt<ast::CallExpression>(call_id));
            const auto scope_id = call_expr.function;

            CHECK(call_expr.arguments.size() == 1);
            const auto& arg_type =
                std::visit([&](const auto& arg) -> auto& { return root_module.get_sema_type(arg); },
                           call_expr.arguments[0]);
            CHECK(&arg_type == &msg_type);

            const auto& scope_expr = helpers::unwrap(
                root_module.ast.get_as_opt<ast::ScopeResolutionExpression>(scope_id));
            const auto& println_fn_type =
                helpers::unwrap(io_module.get_sema_type_opt(scope_expr.inner));
            (void)println_fn_type;
            // TODO: Verify scope resolution
        }
    }

    SECTION("Println function validation") {
        const auto& symbol =
            helpers::unwrap(registry.get_from_opt(*io_module.root_table_idx, "println"));
        CHECK(symbol.is_public(io_module));
        CHECK(symbol.get_kind_opt() == sema::SymbolKind::CALLABLE);
        CHECK(symbol.get_status() == sema::SymbolStatus::RESOLVED);
        const auto node = helpers::unwrap(symbol.as_opt<sema::symbols::Node>());
        CHECK(node.is<ast::DeclStatement>());

        const auto& fn_type = helpers::unwrap(io_module.get_sema_type_opt(node));
        CHECK(fn_type.get_symbol_table_idx_opt() == 3);
        const auto& fn = helpers::unwrap(fn_type.as_opt<sema::types::Function>());
        CHECK(fn.params.size() == 1);

        // Verify the parameter type
        const auto& u8_type = pool[{sema::TypeKind::U8, sema::types::mut::CONSTANT}];
        const auto& u8_slice_type =
            pool[{sema::TypeKind::SLICE, sema::types::mut::CONSTANT, false, u8_type}];

        // Verify the parameter type & symbol
        {
            const auto& param_type = *fn.params[0];
            CHECK(&param_type == &u8_slice_type);

            const auto& param_symbol = helpers::unwrap(registry.get_from_opt(3, "str"));
            CHECK(param_symbol.get_kind_opt() == sema::SymbolKind::VALUE);
            CHECK(param_symbol.get_status() == sema::SymbolStatus::RESOLVED);
            const auto str_param = helpers::unwrap(param_symbol.as_opt<sema::symbols::Parameter>());
            const auto& str_param_type =
                helpers::unwrap(io_module.get_sema_type_opt(str_param.ident));
            CHECK(&str_param_type == &u8_slice_type);
            const auto& str_param_explicit_type =
                helpers::unwrap(io_module.get_sema_type_opt(str_param.explicit_type));
            CHECK(&str_param_explicit_type == &u8_slice_type);
        }

        // Verify the return type
        {
            const auto& return_type = fn.return_type;
            const auto& void_type   = pool[{sema::TypeKind::VOID, sema::types::mut::CONSTANT}];
            CHECK(&return_type == &void_type);
        }
    }
}

} // namespace porpoise::tests

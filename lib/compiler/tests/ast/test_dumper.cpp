#include <sstream>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "helpers/common.hpp"

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

namespace porpoise::tests {

constexpr std::string_view input{R"(
    [_]*N{a, b, c, d, e, 3, "54" };
    a <= b or c == d and e;
    a or b[3uz] == !c;
    continue;
    return enum { RED };
    import std;
    pub import "ast/node.p" as node;
    _ = enum { RED };
    constexpr SIZE := 2uz;
    { a; b; 2; c; };
    while (true) : (i += 1) {a;} else return b;
    var f_ptr: *fn(&a, *mut B, ...): &[0x2uz][N]*E;
    A::B::C;
    packed struct { var a: Foo = bar; const b := fn(*mut this, a: A, b: *B): C { c; }; };
    &a; &mut b; *a;
    match (a) { b => |c| d; e => |_| f; g => h; } else d;
    loop { a; };
    (*arr[i][j]) = 2;
    if (a) { b; } else { c; };
    if constexpr (a) { b; };
    fn(*mut this, a: A, b: *B, ): i32 { c; };
    for (arr, l, p) |i, &mut j, _| { a; } else return b;
    enum : u64 {A = 1ul, B = T, C, };
    @ptrAdd(a, 4uz);
    using T = i32;
    a(&mut r, t, *[N:0]u8);
    .a;
    union { a: i32, b: &mut T, };
    do { a;} while (true);
    1l; 2z; 3u; 'a'; 2.3f; 2.3;
    [1uz]A{a};
    a.b;
    a..b; a..=b;
    var a: []i32;
    {};
    defer 3;
    .{.a = 3};
    TT{.adfasf = a};
    .{};
    union { a: *struct { var b: Foo = bar; }, const b := fn(&self, a: A): C { c; }; };
    enum : i64 { A = 2l, const b := fn(&self, a: A): C { c; }; };
    const a := {};
    test "dump" { import other; std::testing::expect(a == true); }
    var a: std::ArrayList(u8);
    var a: std::Io;
    var a: List(i32);
    break :blk a;
    a: { continue :a; };
)"};

constexpr std::string_view expected{
#include "ast/dump.inc"
};

TEST_CASE("Comprehensive dump") {
    syntax::Parser p{input};
    auto [ast, errors] = p.consume();
    helpers::check_errors<syntax::ParserDiagnostic>(errors);

    std::ostringstream oss;
    ast::ASTDumper     dumper{oss};
    for (const auto& node : ast) { node->accept(dumper); }
    CHECK(expected == oss.view());
}

} // namespace porpoise::tests

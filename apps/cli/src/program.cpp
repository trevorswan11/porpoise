#include <iostream>
#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "program.hpp"

#include "parser/parser.hpp"

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

#include "string.hpp"

namespace conch::cli {

auto Program::interactive() -> void {
    Parser p;

    std::string line;
    while (true) {
        fmt::print(">>> ");
        line.clear();

        if (!std::getline(std::cin, line)) { break; }
        const auto trimmed = string::trim(line);
        if (trimmed == "exit") { break; }

        p.reset(trimmed);
        auto [ast, errors] = p.consume();
        if (!errors.empty()) {
            fmt::println("{}", errors);
        } else {
            ast::ASTDumper dumper{std::cout};
            for (const auto& node : ast) { node->accept(dumper); }
        }
    }
}

} // namespace conch::cli

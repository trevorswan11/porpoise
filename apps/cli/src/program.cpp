#include <iostream>
#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "program.hpp"

#include "ast/dumper.hpp"
#include "ast/node.hpp"

#include "sema/collector.hpp"
#include "sema/pool.hpp"

#include "string.hpp"

namespace porpoise::cli {

auto Program::interactive() -> void {
    std::string line;
    while (true) {
        fmt::print(">>> ");
        line.clear();

        if (!std::getline(std::cin, line)) { break; }
        const auto trimmed = string::trim(line);
        if (trimmed == "exit") { break; }
        if (trimmed.empty()) { continue; }

        // Parsing
        parser_.reset(trimmed);
        auto [ast, parser_errors] = parser_.consume();
        if (!parser_errors.empty()) {
            fmt::println("{}", parser_errors);
            continue;
        } else {
            ast::ASTDumper dumper{std::cout};
            for (const auto& node : ast) { node->accept(dumper); }
        }

        // Sema
        auto [table, pool, sema_errors] = sema::SymbolCollector::collect(ast);
        if (!sema_errors.empty()) {
            fmt::println("{}", sema_errors);
            continue;
        } else {
            auto _ = std::move(pool);
            fmt::println("{} symbols collected", table.size());
        }
    }
}

} // namespace porpoise::cli

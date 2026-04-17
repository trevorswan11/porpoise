#include <iostream>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "arguments/ast_dump.hpp"

#include "ast/dumper.hpp"
#include "ast/node.hpp"

#include "sema/analyzer.hpp"

#include "string.hpp"

namespace porpoise::driver {

auto AstDump::run() -> void {
    while (true) {
        fmt::print(">>> ");
        line_.clear();

        if (!std::getline(std::cin, line_)) { break; }
        const auto trimmed = string::trim(line_);
        if (trimmed == "exit") { break; }
        if (trimmed.empty()) { continue; }

        // Parsing
        parser_.reset(trimmed);
        auto [ast, parser_errors] = parser_.consume();
        if (!parser_errors.empty()) {
            fmt::println(std::cerr, "{}", parser_errors);
            continue;
        } else {
            ast::ASTDumper dumper{std::cout};
            for (const auto& node : ast) { node->accept(dumper); }
        }

        // Sema
        sema::Analyzer analyzer{std::move(ast)};
        const auto     idx = analyzer.collect_symbols();
        if (analyzer.has_diagnostics()) {
            fmt::println(std::cerr, "{}", analyzer.get_diagnostics());
            continue;
        } else {
            fmt::println("{} total tables, {} top-level symbols collected",
                         analyzer.get_registry().size(),
                         analyzer.get_table(idx).size());
        }
    }
}

} // namespace porpoise::driver

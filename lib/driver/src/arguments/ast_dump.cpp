#include <iostream>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "arguments/ast_dump.hpp"

#include "ast/dumper.hpp"
#include "ast/node.hpp"

#include "sema/analyzer.hpp"
#include "sema/module/memory_loader.hpp"
#include "sema/module/module.hpp"

#include "string.hpp"

namespace porpoise::driver {

auto AstDump::run() -> void {
    const std::filesystem::path stdin_path = "stdin.porp";
    while (true) {
        fmt::print(">>> ");
        line_.clear();

        if (!std::getline(std::cin, line_)) { break; }
        auto trimmed = string::trim(line_);
        if (trimmed == "exit") { break; }
        if (trimmed.empty()) { continue; }

        sema::mod::MemoryLoader  loader;
        sema::mod::ModuleManager manager{loader};
        loader.add(stdin_path, std::string{trimmed});

        sema::Analyzer analyzer{manager, std::cerr, true};
        if (!analyzer.analyze(stdin_path)) {
            fmt::println(std::cerr, "Failed to load input from stdin");
            break;
        }

        // Print debug information from each stage
        const auto stdin_mod = *manager.try_get_file_module(stdin_path);
        if (stdin_mod->is_errored()) { continue; }

        ast::ASTDumper dumper{std::cout};
        for (const auto& node : stdin_mod->tree) { node->accept(dumper); }

        if (stdin_mod->is_poisoned()) { continue; }
        fmt::println("{} total tables, {} top-level symbols collected",
                     analyzer.get_registry().size(),
                     analyzer.get_table(stdin_mod->root_table_idx).size());
    }
}

} // namespace porpoise::driver

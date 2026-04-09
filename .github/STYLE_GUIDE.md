# Style Guide

Porpoise adheres to a somewhat strict style guide to enhance readability.

## Formatting
`clang-format` is used for code formatting all C++ code, while Zig's builtin formatter is used for formatting all Zig code. While the latter of the two comes for free with a proper Zig installation, the former can be quite involved. In order to invoke the tool automatically, porpoise's build system exposes a step executed with `zig build fmt`. This is incredibly resource intensive, building the entire `clang-format` tool from scratch (though it does cache the binary). If you do not have the compute to front this cost, that is perfectly acceptable. A maintainer will be more than happy to format your changes on your behalf using their system, eliminating any worry of PR rejection.

Upon PR creation and workflow approval, GitHub actions will run `zig build fmt-check`, an equally-intensive step that performs a dry run of `clang-format` on the codebase. This can also be invoked on your machine if you are interested in confirming the state of your local version of the codebase.

## General Naming Conventions

## Language-Specific Conventions

### C++

### Zig
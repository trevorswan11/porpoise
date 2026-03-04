//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/ASTMatchers
pub const root = "clang/lib/ASTMatchers/";
pub const sources = [_][]const u8{
    "ASTMatchFinder.cpp",
    "ASTMatchersInternal.cpp",
    "GtestMatchers.cpp",
    "LowLevelHelpers.cpp",
};

pub const dynamic_root = root ++ "Dynamic";
pub const dynamic_sources = [_][]const u8{
    "Diagnostics.cpp",
    "Marshallers.cpp",
    "Parser.cpp",
    "Registry.cpp",
    "VariantValue.cpp",
};

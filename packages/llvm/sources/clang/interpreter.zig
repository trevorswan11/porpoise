//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Interpreter/CMakeLists.txt
pub const root = "clang/lib/Interpreter";
pub const sources = [_][]const u8{
    "DeviceOffload.cpp",
    "CodeCompletion.cpp",
    "IncrementalExecutor.cpp",
    "IncrementalParser.cpp",
    "Interpreter.cpp",
    "InterpreterValuePrinter.cpp",
    "InterpreterUtils.cpp",
    "RemoteJITUtils.cpp",
    "Value.cpp",
};

//! Clang Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

const LLVMBuilder = @import("LLVMBuilder.zig");

const coff = @import("sources/lld/coff.zig");
const common = @import("sources/lld/common.zig");
const elf = @import("sources/lld/elf.zig");
const macho = @import("sources/lld/macho.zig");
const mingw = @import("sources/lld/mingw.zig");
const wasm = @import("sources/lld/wasm.zig");

const Artifact = LLVMBuilder.Artifact;
const ArtifactCreateConfig = LLVMBuilder.ArtifactCreateConfig;
const ArtifactWithGen = LLVMBuilder.ArtifactWithGen;

const Metadata = struct {
    root: std.Build.LazyPath,
    lld_include: std.Build.LazyPath,
    vcs_version: *std.Build.Step.ConfigHeader,
};

/// Artifacts compiled for the actual target, associated with the lld subproject of llvm
const LLDTargetArtifacts = struct {
    const Common = struct {
        core_lib: Artifact = undefined,
        version_inc: *std.Build.Step.ConfigHeader = undefined,
    };

    coff: ArtifactWithGen = .{},
    common: Common = .{},
    elf: ArtifactWithGen = .{},
    macho: ArtifactWithGen = .{},
    mingw: ArtifactWithGen = .{},
    wasm: ArtifactWithGen = .{},
};

const Self = @This();

b: *std.Build,
llvm: *const LLVMBuilder,
metadata: Metadata,

lld_artifacts: LLDTargetArtifacts = .{},

/// This is only true once `build` is called and exits successfully
complete: bool = false,

/// Creates a new lld builder from a potentially unfinished LLVM
pub fn init(llvm: *const LLVMBuilder) *Self {
    const b = llvm.b;
    const self = b.allocator.create(Self) catch @panic("OOM");

    self.* = .{
        .b = b,
        .llvm = llvm,
        .metadata = .{ .root = llvm.metadata.root.dupe(b), .lld_include = llvm.metadata.root.path(b, "lld/include"), .vcs_version = b.addConfigHeader(.{ .include_path = "VCSVersion.inc" }, .{
            .LLVM_REVISION = LLVMBuilder.llvm_revision,
            .LLD_REVISION = LLVMBuilder.llvm_revision,
            .LLVM_REPOSITORY = "https://github.com/llvm/llvm-project.git",
            .LLD_REPOSITORY = "https://github.com/llvm/llvm-project.git",
        }) },
    };
    return self;
}

/// This must be called once llvm has been built with its `build`
pub fn build(self: *Self) void {
    if (!self.llvm.complete) {
        @panic("Misconfigured build script - LLVM was not built");
    }

    self.complete = true;
}

/// Creates a module and corresponding library, defaulting to the target platform.
/// `llvm/include` and `lld/include` are implicitly included here
pub fn createLLDLibrary(self: *const Self, config: ArtifactCreateConfig) Artifact {
    const lib = self.llvm.createLLVMLibrary(config);
    lib.root_module.addIncludePath(self.metadata.lld_include);
    return lib;
}

pub fn allArtifacts(self: *const Self) []Artifact {
    var all_artifacts: std.ArrayList(Artifact) = .empty;
    all_artifacts.appendSlice(self.b.allocator, &.{
        self.lld_artifacts.common.core_lib,
        self.lld_artifacts.coff.core_lib,
        self.lld_artifacts.elf.core_lib,
        self.lld_artifacts.macho.core_lib,
        self.lld_artifacts.mingw.core_lib,
        self.lld_artifacts.wasm.core_lib,
    }) catch @panic("OOM");
    return all_artifacts.items;
}

pub fn allIncludePaths(self: *const Self) LLVMBuilder.AllIncludes {
    var all_includes: std.ArrayList(std.Build.LazyPath) = .empty;
    all_includes.appendSlice(self.b.allocator, &.{
        self.metadata.lld_include,
        self.lld_artifacts.coff.gen.getDirectory(),
        self.lld_artifacts.elf.gen.getDirectory(),
        self.lld_artifacts.macho.gen.getDirectory(),
        self.lld_artifacts.mingw.gen.getDirectory(),
        self.lld_artifacts.wasm.gen.getDirectory(),
    }) catch @panic("OOM");

    var all_config_headers: std.ArrayList(*std.Build.Step.ConfigHeader) = .empty;
    all_config_headers.appendSlice(self.b.allocator, &.{
        self.metadata.vcs_version,
        self.lld_artifacts.common.version_inc,
    }) catch @panic("OOM");

    return .{
        .includes = all_includes.items,
        .config_headers = all_config_headers.items,
    };
}

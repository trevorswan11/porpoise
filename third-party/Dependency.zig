const std = @import("std");

pub const Config = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

pub const Artifact = *std.Build.Step.Compile;

upstream: *std.Build.Dependency,
artifact: *std.Build.Step.Compile,

pub fn addFrameworkSearchPaths(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .macos) return;
    const b = mod.owner;
    if (b.graph.environ_map.get("SDKROOT")) |sdkroot| {
        mod.addFrameworkPath(.{ .cwd_relative = b.fmt("{s}/System/Library/Frameworks", .{sdkroot}) });
        mod.addSystemIncludePath(.{ .cwd_relative = b.fmt("{s}/usr/include", .{sdkroot}) });
    }
}

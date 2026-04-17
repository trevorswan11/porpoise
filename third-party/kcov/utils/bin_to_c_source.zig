//! This is bin-to-c-source.py ported to Zig:
//! https://github.com/SimonKagstrom/kcov/blob/master/src/bin-to-c-source.py
//! https://github.com/allyourcodebase/kcov/blob/master/bin_to_c_source.zig
const std = @import("std");

pub fn main(init: std.process.Init) !void {
    const allocator = init.arena.allocator();
    const io = init.io;

    var buffer: [4096]u8 = undefined;
    var stdout_writer = std.Io.File.stdout().writer(io, &buffer);
    const stdout = &stdout_writer.interface;

    const args = try init.minimal.args.toSlice(allocator);
    if (args.len < 3 or (args.len - 1) % 2 != 0) {
        std.debug.print("Usage: {s} [file] [base-name] [<file2> <base-name2>]\n", .{args[0]});
        std.process.exit(1);
    }

    try stdout.writeAll(
        \\#include <stdint.h>
        \\#include <stdlib.h>
        \\#include <generated-data-base.hh>
        \\using namespace kcov;
        \\
    );

    var i: usize = 1;
    while (i + 1 < args.len) : (i += 2) {
        const file = args[i];
        const base_name = args[i + 1];

        const data = try std.Io.Dir.cwd().readFileAlloc(io, file, allocator, .unlimited);
        try generate(stdout, data, base_name);
    }

    try stdout.flush();
}

fn generate(writer: *std.Io.Writer, data: []const u8, base_name: []const u8) !void {
    try writer.print("const uint8_t {s}_data_raw[] = {{\n", .{base_name});

    for (data, 0..) |c, i| {
        // more optimized version of: try writer.print("0x{x:0>2},", .{c});
        const charset = "0123456789abcdef";
        try writer.writeAll(&.{ '0', 'x', charset[c >> 4], charset[c & 15], ',' });
        if (i % 20 == 19) try writer.writeByte('\n');
    }

    try writer.print(
        \\
        \\}};
        \\GeneratedData {0s}_data({0s}_data_raw, sizeof({0s}_data_raw));
        \\
    , .{base_name});
}

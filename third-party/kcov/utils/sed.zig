//! https://github.com/allyourcodebase/binutils/blob/master/find_replace.zig
const std = @import("std");

pub fn main(init: std.process.Init) !void {
    const allocator = init.arena.allocator();
    const io = init.io;

    const args = try init.minimal.args.toSlice(allocator);
    if (args.len != 5) {
        std.debug.print("usage: {s} [input_file] [output_file] [before] [after]\n", .{args[0]});
        return error.UsageError;
    }

    const input_filename = args[1];
    const output_filename = args[2];
    const before = args[3];
    const after = args[4];

    const input = try std.Io.Dir.cwd().readFileAlloc(io, input_filename, allocator, .unlimited);
    const output = try std.mem.replaceOwned(u8, allocator, input, before, after);

    try std.Io.Dir.cwd().writeFile(io, .{
        .sub_path = output_filename,
        .data = output,
    });
}

const std = @import("std");
const testing = std.testing;

extern fn launch(c_int, [*c][*c]u8) c_int;

var instrumentor: Instrumentor = undefined;

pub fn main(init: std.process.Init) !void {
    var gpa: Instrumentor.Gpa = .init;
    const allocator = gpa.allocator();

    instrumentor = .init(allocator, init.io);
    defer instrumentor.deinit(&gpa);

    const args = try instrumentor.getCArgs(init.minimal.args);
    std.log.info("{s}", .{args.argv[0]});
    const proc: usize = @intCast(launch(args.argc, args.argv));

    const result: u8 = @intCast(gpa.detectLeaks() | proc);
    instrumentor.report();
    std.process.exit(result);
}

const Instrumentor = struct {
    const internal_allocator = std.heap.c_allocator;

    const AllocHeader = extern struct {
        size: usize,
        offset: usize,
        requested: usize,
        magic: usize = header_magic,

        pub fn valid(self: *const AllocHeader) bool {
            return self.offset < self.size and self.magic == header_magic;
        }
    };

    const header_magic = 0xDEADBEEF;
    const header_size = @sizeOf(AllocHeader);

    const Gpa = std.heap.DebugAllocator(.{
        .thread_safe = true,
        .stack_trace_frames = if (std.debug.sys_can_stack_trace) 10 else 0,
    });

    allocator: std.mem.Allocator,
    io: std.Io,

    args: ?struct {
        zig_conv: std.ArrayList([:0]u8) = .empty,
        c_conv: [][*c]u8,
    } = null,

    total_nodes: std.atomic.Value(u64) = .init(0),
    total_alloc: std.atomic.Value(u64) = .init(0),
    node_counter: std.atomic.Value(u64) = .init(0),
    byte_counter: std.atomic.Value(u64) = .init(0),

    live_lock: std.Io.Mutex = .init,
    live_allocations: std.AutoHashMap(usize, void),

    pub fn init(allocator: std.mem.Allocator, io: std.Io) Instrumentor {
        return .{
            .allocator = allocator,
            .io = io,
            .live_allocations = .init(internal_allocator),
        };
    }

    pub fn deinit(self: *Instrumentor, gpa: ?*Gpa) void {
        self.live_allocations.deinit();
        if (self.args) |*args| {
            freeArgs(&args.zig_conv);
            internal_allocator.free(args.c_conv);
        }
        if (gpa) |g| _ = g.deinit();
    }

    fn freeArgs(args: *std.ArrayList([:0]u8)) void {
        for (args.items) |arg| {
            internal_allocator.free(arg);
        }
        args.deinit(internal_allocator);
        args.* = .empty;
    }

    pub fn getCArgs(
        self: *Instrumentor,
        args: std.process.Args,
    ) !struct { argc: c_int, argv: [*c][*c]u8 } {
        var args_iter = try args.iterateAllocator(internal_allocator);
        defer args_iter.deinit();

        var zig_args: std.ArrayList([:0]u8) = try .initCapacity(internal_allocator, 2);
        errdefer freeArgs(&zig_args);

        while (args_iter.next()) |arg| {
            const mut_arg = try internal_allocator.dupeZ(u8, arg);
            try zig_args.append(internal_allocator, mut_arg);
        }

        const c_args = try internal_allocator.alloc([*c]u8, zig_args.items.len);
        for (zig_args.items, 0..) |arg, i| {
            c_args[i] = arg.ptr;
        }

        self.args = .{ .zig_conv = zig_args, .c_conv = c_args };
        return .{ .argc = @intCast(c_args.len), .argv = c_args.ptr };
    }

    const AllocError = error{ AllocationFailed, PtrStoreFailed, LockFailed };

    fn alloc(self: *Instrumentor, size: usize) AllocError!*anyopaque {
        const alignment = comptime @max(16, @alignOf(std.c.max_align_t));
        const total = size + alignment + header_size;

        _ = self.total_nodes.fetchAdd(1, .acq_rel);
        _ = self.total_alloc.fetchAdd(@intCast(total), .acq_rel);
        _ = self.node_counter.fetchAdd(1, .acq_rel);

        const mem = self.allocator.alloc(u8, total) catch return error.AllocationFailed;
        errdefer self.allocator.free(mem);
        const base_ptr = mem.ptr;

        const aligned_ptr = blk: {
            const ptr = std.mem.alignForward(
                usize,
                @intFromPtr(base_ptr) + header_size,
                alignment,
            );

            const result = try self.putKey(ptr);
            break :blk result orelse return error.PtrStoreFailed;
        };

        const header = @as(
            *AllocHeader,
            @ptrFromInt(aligned_ptr - header_size),
        );
        header.* = .{
            .size = total,
            .offset = aligned_ptr - @intFromPtr(base_ptr),
            .requested = size,
        };
        _ = self.byte_counter.fetchAdd(header.requested, .acq_rel);
        return @ptrFromInt(aligned_ptr);
    }

    const DeallocError = error{ InvalidFree, HeapCorruption, LockFailed };

    fn dealloc(self: *Instrumentor, ptr: ?*anyopaque) DeallocError!void {
        const p = ptr orelse return;

        // Locks are manual here to prevent two lock/unlock cycles in one function
        self.live_lock.lock(self.io) catch return error.LockFailed;
        defer self.live_lock.unlock(self.io);

        const key = @intFromPtr(p);
        if (!self.containsKey(key)) {
            return error.InvalidFree;
        }

        const header = blk: {
            const header = @as(
                *const AllocHeader,
                @ptrFromInt(key - header_size),
            );

            if (!header.valid()) {
                return error.HeapCorruption;
            }
            break :blk header;
        };

        _ = self.byte_counter.fetchSub(header.requested, .acq_rel);
        _ = self.node_counter.fetchSub(1, .acq_rel);
        _ = self.removeKey(key);

        const base_ptr = key - header.offset;
        const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
        self.allocator.free(slice);
    }

    // Handles the locks itself
    pub fn putKey(self: *Instrumentor, key: usize) error{LockFailed}!?usize {
        self.live_lock.lock(self.io) catch return error.LockFailed;
        defer self.live_lock.unlock(self.io);

        if (self.live_allocations.contains(key)) return null;
        self.live_allocations.put(key, {}) catch return null;
        return key;
    }

    // Does not handle the locks itself!
    pub fn containsKey(self: *Instrumentor, key: usize) bool {
        return self.live_allocations.contains(key);
    }

    // Does not handle the locks itself!
    pub fn removeKey(self: *Instrumentor, key: usize) bool {
        return self.live_allocations.remove(key);
    }

    pub fn report(self: *Instrumentor) void {
        const allocated, const alloc_unit = formatBytes(self.total_alloc.load(.acquire));
        const remaining, const rem_unit = formatBytes(self.byte_counter.load(.acquire));

        std.log.info("{d} nodes malloced for {d:.3} {s}", .{
            self.total_nodes.load(.acquire),
            allocated,
            alloc_unit,
        });
        std.log.info("{d} leak(s) for {d:.3} total leaked {s}\n", .{
            self.node_counter.load(.acquire),
            remaining,
            rem_unit,
        });
    }

    fn formatBytes(bytes: u64) struct { f64, []const u8 } {
        const float_bytes: f64 = @floatFromInt(bytes);
        if (bytes >= 1_000_000_000) return .{ float_bytes / 1_000_000_000.0, "GB" };
        if (bytes >= 1_000_000) return .{ float_bytes / 1_000_000.0, "MB" };
        if (bytes >= 1_000) return .{ float_bytes / 1_000.0, "KB" };
        return .{ float_bytes, "bytes" };
    }

    test formatBytes {
        try testing.expectEqual(.{ 500.0, "bytes" }, formatBytes(500));
        try testing.expectEqual(.{ 2.5, "KB" }, formatBytes(2500));
        try testing.expectEqual(.{ 5.5, "MB" }, formatBytes(5_500_000));
        try testing.expectEqual(.{ 3.7, "GB" }, formatBytes(3_700_000_000));
        try testing.expectEqual(.{ 1.0, "KB" }, formatBytes(1000));
        try testing.expectEqual(.{ 1.0, "MB" }, formatBytes(1_000_000));
        try testing.expectEqual(.{ 1.0, "GB" }, formatBytes(1_000_000_000));
    }
};

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    return instrumentor.alloc(size) catch null;
}

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    instrumentor.dealloc(ptr) catch |err| switch (err) {
        error.InvalidFree => @panic("Double or invalid free detected"),
        error.HeapCorruption => @panic("Heap corruption detected: allocated block has malformed header"),
        error.LockFailed => @panic("IO Error: Failed to obtain lock"),
    };
}

test "Correct allocation pipeline" {
    var inst: Instrumentor = .init(testing.allocator, testing.io);
    defer inst.deinit(null);

    for ([_]usize{ 1, 4, 16, 31, 65, 1024 }) |size| {
        const ptr = try inst.alloc(size);
        defer inst.dealloc(ptr) catch {};

        // Verify header and alignment
        const addr = @intFromPtr(ptr);
        try testing.expectEqual(addr % 16, 0);

        const header_ptr = @as(*Instrumentor.AllocHeader, @ptrFromInt(addr - Instrumentor.header_size));
        try testing.expectEqual(Instrumentor.header_magic, header_ptr.magic);
        try testing.expectEqual(size, header_ptr.requested);
        try testing.expect(header_ptr.offset >= Instrumentor.header_size);

        // The pointer should be rw
        const user_slice = @as([*]u8, @ptrFromInt(addr))[0..size];
        @memset(user_slice, 0xAA);
        for (user_slice) |byte| {
            try testing.expectEqual(byte, 0xAA);
        }
    }
}

test "Detect double free" {
    var inst: Instrumentor = .init(testing.allocator, testing.io);
    defer inst.deinit(null);

    const ptr = try inst.alloc(32);
    try inst.dealloc(ptr);
    try testing.expectError(error.InvalidFree, inst.dealloc(ptr));
}

test "Detect invalid free" {
    var inst: Instrumentor = .init(testing.allocator, testing.io);
    defer inst.deinit(null);

    const ptr = try testing.allocator.alloc(u8, 32);
    defer testing.allocator.free(ptr);
    try testing.expectError(error.InvalidFree, inst.dealloc(@ptrCast(ptr)));
}

test "Detect header corruption" {
    var inst: Instrumentor = .init(testing.allocator, testing.io);
    defer inst.deinit(null);
    const ptr = try inst.alloc(32);

    const addr = @intFromPtr(ptr);
    const header_ptr = @as(*Instrumentor.AllocHeader, @ptrFromInt(addr - Instrumentor.header_size));
    header_ptr.magic = 0xBADF00D;
    try testing.expectError(error.HeapCorruption, inst.dealloc(ptr));

    header_ptr.magic = Instrumentor.header_magic;
    try inst.dealloc(ptr);
}

test "Concurrent allocation stress" {
    var inst: Instrumentor = .init(testing.allocator, testing.io);
    defer inst.deinit(null);
    var threads: [4]std.Thread = undefined;

    const Runner = struct {
        const ops_per_thread = 1000;
        const allocator = testing.allocator;

        fn run(mock: *Instrumentor) !void {
            var ptrs: std.ArrayList(*anyopaque) = .empty;
            defer {
                for (ptrs.items) |p| {
                    mock.dealloc(p) catch {};
                }
                ptrs.deinit(allocator);
            }

            for (0..ops_per_thread) |_| {
                try ptrs.append(allocator, try mock.alloc(8));
            }
        }
    };

    for (&threads) |*t| {
        t.* = try .spawn(.{}, Runner.run, .{&inst});
    }
    for (threads) |t| t.join();

    try testing.expectEqual(0, inst.node_counter.load(.acquire));
    try testing.expectEqual(0, inst.byte_counter.load(.acquire));
}

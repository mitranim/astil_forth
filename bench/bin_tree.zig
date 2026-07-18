// BOT-TRANSLATED from reg-CC file.

const std = @import("std");

const Allocator = std.mem.Allocator;

const Node = struct {
    left: ?*Node,
    right: ?*Node,

    fn init(alc: Allocator, depth: u32) !*Node {
        const self = try alc.create(Node);
        if (depth == 0) {
            self.* = .{ .left = null, .right = null };
        } else {
            self.* = .{
                .left = try init(alc, depth - 1),
                .right = try init(alc, depth - 1),
            };
        }
        return self;
    }

    fn count(self: *const Node) u64 {
        return 1 +
            (if (self.left) |left| left.count() else 0) +
            (if (self.right) |right| right.count() else 0);
    }
};

pub fn main() !void {
    const min_depth = 4;
    const max_depth = 18;
    const stretch_depth = max_depth + 1;

    var long_lived_arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer long_lived_arena.deinit();
    const long_lived_tree = try Node.init(long_lived_arena.allocator(), max_depth);

    var scratch_arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer scratch_arena.deinit();

    var out: [1024]u8 = undefined;
    var writer: std.Io.Writer = .fixed(&out);

    {
        const tree = try Node.init(scratch_arena.allocator(), stretch_depth);
        try writer.print(
            "stretch tree of depth {d}; count: {d}\n",
            .{ stretch_depth, tree.count() },
        );
        if (!scratch_arena.reset(.retain_capacity)) return error.OutOfMemory;
    }

    var depth: u32 = min_depth;
    while (depth <= max_depth) : (depth += 2) {
        var count: u64 = 0;
        const iters: u64 = @as(u64, 1) << @intCast(max_depth - depth + min_depth);

        for (0..iters) |_| {
            const tree = try Node.init(scratch_arena.allocator(), depth);
            count += tree.count();
            if (!scratch_arena.reset(.retain_capacity)) return error.OutOfMemory;
        }

        try writer.print(
            "{d} trees of depth {d}; count: {d}\n",
            .{ iters, depth, count },
        );
    }

    try writer.print(
        "long lived tree of depth {d}; count: {d}\n",
        .{ max_depth, long_lived_tree.count() },
    );

    const expected =
        "stretch tree of depth 19; count: 1048575\n" ++
        "262144 trees of depth 4; count: 8126464\n" ++
        "65536 trees of depth 6; count: 8323072\n" ++
        "16384 trees of depth 8; count: 8372224\n" ++
        "4096 trees of depth 10; count: 8384512\n" ++
        "1024 trees of depth 12; count: 8387584\n" ++
        "256 trees of depth 14; count: 8388352\n" ++
        "64 trees of depth 16; count: 8388544\n" ++
        "16 trees of depth 18; count: 8388592\n" ++
        "long lived tree of depth 18; count: 524287\n";

    if (!std.mem.eql(u8, writer.buffered(), expected)) return error.WrongOutput;
}

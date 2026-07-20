// BOT-GENERATED

const std = @import("std");

// Zig 0.16's default 256 KiB alternate signal stack becomes TLS in every
// thread. Under benchmark load it raised peak RSS from about 66 MiB to
// 1158 MiB; it is not connection state.
pub const std_options: std.Options = .{ .signal_stack_size = null };
const port = 19777;
const pthread_create_detached: c_int = 2;

extern fn pthread_attr_setdetachstate(
    attr: *std.c.pthread_attr_t,
    detach_state: c_int,
) c_int;

const Ctx = struct {
    alc: std.mem.Allocator,
    io: std.Io,
    stream: std.Io.net.Stream,
};

fn serve(ctx: *Ctx) !void {
    defer ctx.stream.close(ctx.io);
    var read_buffer: [1]u8 = undefined;
    var write_buffer: [1]u8 = undefined;
    var reader = ctx.stream.reader(ctx.io, &read_buffer);
    var writer = ctx.stream.writer(ctx.io, &write_buffer);
    try writer.interface.writeAll("R");
    try writer.interface.flush();
    if (try reader.interface.takeByte() != 'D') {
        return error.InvalidData;
    }
    try writer.interface.writeAll("D");
    try writer.interface.flush();
}

fn start(raw: ?*anyopaque) callconv(.c) ?*anyopaque {
    const ctx: *Ctx = @ptrCast(@alignCast(raw));
    defer ctx.alc.destroy(ctx);
    serve(ctx) catch std.process.exit(1);
    return null;
}

pub fn main(init: std.process.Init) !void {
    const address: std.Io.net.IpAddress = .{ .ip4 = .loopback(port) };

    var server = try address.listen(init.io, .{ .reuse_address = true });
    defer server.deinit(init.io);

    var attr: std.c.pthread_attr_t = undefined;
    if (std.c.pthread_attr_init(&attr) != .SUCCESS) {
        return error.ThreadAttributeInitFailed;
    }
    defer _ = std.c.pthread_attr_destroy(&attr);

    if (pthread_attr_setdetachstate(&attr, pthread_create_detached) != 0) {
        return error.ThreadAttributeDetachFailed;
    }

    while (true) {
        const stream = try server.accept(init.io);
        const ctx = init.gpa.create(Ctx) catch |err| {
            stream.close(init.io);
            return err;
        };

        ctx.* = .{ .alc = init.gpa, .io = init.io, .stream = stream };

        // This attribute changes only detach state, preserving OS-default,
        // lazily paged stacks. std.Thread has no OS-default-stack option.
        var thread: std.c.pthread_t = undefined;
        if (std.c.pthread_create(&thread, &attr, start, ctx) != .SUCCESS) {
            stream.close(init.io);
            init.gpa.destroy(ctx);
            return error.ThreadCreationFailed;
        }
    }
}

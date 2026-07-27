// BOT-GENERATED

const std = @import("std");

const buffer_size = 64 * 1024;

fn copy(
    io: std.Io,
    file: std.Io.File,
    output: *std.Io.Writer,
    buffer: []u8,
) !void {
    var input = file.readerStreaming(io, buffer);
    _ = try input.interface.streamRemaining(output);
}

pub fn main(init: std.process.Init) !void {
    const args = try init.minimal.args.toSlice(init.arena.allocator());

    var input_buffer: [buffer_size]u8 = undefined;
    var output_buffer: [buffer_size]u8 = undefined;
    var output = std.Io.File.stdout().writerStreaming(
        init.io,
        &output_buffer,
    );

    if (args.len == 1) {
        try copy(
            init.io,
            std.Io.File.stdin(),
            &output.interface,
            &input_buffer,
        );
    }

    for (args[1..]) |arg| {
        if (std.mem.eql(u8, arg, "-")) {
            try copy(
                init.io,
                std.Io.File.stdin(),
                &output.interface,
                &input_buffer,
            );
            continue;
        }

        const input = try std.Io.Dir.cwd().openFile(init.io, arg, .{});
        defer input.close(init.io);
        try copy(init.io, input, &output.interface, &input_buffer);
    }

    try output.interface.flush();
}

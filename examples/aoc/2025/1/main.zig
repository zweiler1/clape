const std = @import("std");

fn mod(x: i64, y: i64) i64 {
    return @rem(@rem(x, y) + y, y);
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    var threaded = std.Io.Threaded.init(allocator, .{});
    defer threaded.deinit();
    const io = threaded.io();

    const file = try std.Io.Dir.cwd().openFile(io, "input.txt", .{});
    defer file.close(io);

    const stat = try file.stat(io);
    const contents = try allocator.alloc(u8, stat.size);
    defer allocator.free(contents);
    _ = try file.readPositionalAll(io, contents, 0);

    var nums: [5000]i64 = undefined;
    var n: usize = 0;

    var i: usize = 0;
    while (i < contents.len) {
        const start = i;
        while (i < contents.len and contents[i] != '\n') : (i += 1) {}
        const end = i;

        if (end == start) {
            nums[n] = 0;
            n += 1;
        } else {
            const sign: i64 = if (contents[start] == 'L') -1 else 1;
            var val: i64 = 0;
            var j: usize = start + 1;
            while (j < end) : (j += 1) {
                val = val * 10 + (contents[j] - '0');
            }
            nums[n] = val * sign;
            n += 1;
        }
        if (i >= contents.len) break;
        i += 1;
    }
    if (contents.len > 0 and contents[contents.len - 1] == '\n') {
        nums[n] = 0;
        n += 1;
    }

    var dial: i64 = 50;
    var count: i64 = 0;
    for (nums[0..n]) |x| {
        dial = mod(dial + x, 100);
        if (dial == 0) count += 1;
    }

    std.debug.print("part_1: {d}\n", .{count});

    dial = 50;
    count = 0;
    for (nums[0..n]) |x| {
        const s = dial + x;
        if (dial > 0 and x < 0 and s < 0) count += 1;
        const abs_s = if (s >= 0) s else -s;
        const adj = @max(abs_s - 1, 0);
        count += @divTrunc(adj, 100);
        dial = mod(dial + x, 100);
        if (dial == 0) count += 1;
    }
    std.debug.print("part_2: {d}\n", .{count});
}

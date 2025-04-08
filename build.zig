//! Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

const std = @import("std");
const builtin = @import("builtin");

const c_flags = &[_][]const u8{
    // lang
    "-std=c11",
    // warnings
    "-Wall",
    "-Wextra",
    "-pedantic",
    // flags
    "-fno-pie",
    "-fno-stack-protector",
    "-fno-omit-frame-pointer",
    "-ffreestanding",
    "-fno-builtin",
    // symbols
    "-ggdb",
};

const kernel_src = &[_][]const u8{
    "kernel/entry.S", // must be first
    "kernel/main.c", // main function
    "kernel/cpu/cpu.c",
    "kernel/cpu/fpu.c",
    "kernel/cpu/idt.c",
    "kernel/cpu/idt.S",
    "kernel/cpu/pic.c",
    "kernel/drivers/acpi.c",
    "kernel/drivers/clock.c",
    "kernel/drivers/drivers.c",
    "kernel/drivers/pci.c",
    "kernel/drivers/tty.c",
    "kernel/drivers/uart.c",
    "kernel/fs/fs.c",
    "kernel/lib/atox.c",
    "kernel/lib/bound.c",
    "kernel/lib/btoa.c",
    "kernel/lib/ctoi.c",
    "kernel/lib/isdigit.c",
    "kernel/lib/isspace.c",
    "kernel/lib/itoc.c",
    "kernel/lib/kalloc.c",
    "kernel/lib/kprintf.c",
    "kernel/lib/memcmp.c",
    "kernel/lib/memcpy.c",
    "kernel/lib/memcpyv.c",
    "kernel/lib/memmove.c",
    "kernel/lib/memmovev.c",
    "kernel/lib/memset.c",
    "kernel/lib/memsetv.c",
    "kernel/lib/panic.c",
    "kernel/lib/stpcpy.c",
    "kernel/lib/stpncpy.c",
    "kernel/lib/strcat.c",
    "kernel/lib/strcpy.c",
    "kernel/lib/strlen.c",
    "kernel/lib/strncmp.c",
    "kernel/lib/strncpy.c",
    "kernel/lib/strtoux.c",
    "kernel/lib/strtox.c",
    "kernel/lib/uxtoa.c",
    "kernel/lib/xtoa.c",
    "kernel/mboot/mboot.c",
    "kernel/mboot/mmap.c",
    "kernel/mboot/rsdp.c",
    "kernel/memory/memory.c",
    "kernel/memory/paging.c",
    "kernel/memory/physalloc.c",
    "kernel/memory/virtalloc.c",
};

const Prog = struct {
    name: []const u8,
    source: []const []const u8,
};

const AddSourcesOpts = struct { exe: *std.Build.Step.Compile, sources: []const []const []const u8, c_flags: []const []const u8 };

fn add_sources(b: *std.Build, opts: AddSourcesOpts) void {
    // add asm and c source files
    for (opts.sources) |source| {
        for (source) |file| {
            if (std.mem.endsWith(u8, file, ".c")) {
                // c file
                opts.exe.addCSourceFile(.{ .file = b.path(file), .flags = opts.c_flags });
            } else {
                // assembly file
                opts.exe.addAssemblyFile(b.path(file));
            }
        }
    }
}

const BuildKernBinaryOpts = struct {
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    sources: []const []const []const u8,
    linker: ?[]const u8 = null,
    entry: []const u8 = "_start",
    strip: bool = false,
    include: ?[]const u8 = null,
};

fn build_kern_binary(b: *std.Build, opts: BuildKernBinaryOpts) void {
    // create compile step
    const exe = b.addExecutable(.{
        .name = opts.name,
        .target = opts.target,
        .optimize = opts.optimize,
        .strip = opts.strip,
    });

    // add include path
    if (opts.include != null) {
        exe.addIncludePath(b.path(opts.include.?));
    }

    // set entry
    exe.entry = .{ .symbol_name = opts.entry };

    // add asm and c source files
    add_sources(b, .{
        .exe = exe,
        .sources = opts.sources,
        .c_flags = c_flags,
    });

    if (opts.linker != null) {
        exe.setLinkerScript(b.path(opts.linker.?));
    }

    const step = b.addInstallArtifact(exe, .{ .dest_dir = .{
        .override = .{ .custom = "../bin" },
    } });
    b.getInstallStep().dependOn(&step.step);
}

const BuildNativeBinaryOpts = struct {
    name: []const u8,
    optimize: std.builtin.OptimizeMode,
    sources: []const []const []const u8,
};

fn build_native_binary(b: *std.Build, opts: BuildNativeBinaryOpts) void {
    // create compile step
    const exe = b.addExecutable(.{
        .name = opts.name,
        .target = b.graph.host,
        .optimize = opts.optimize,
        .strip = true,
    });

    // include libc
    exe.linkLibC();

    // add asm and c source files
    add_sources(b, .{
        .exe = exe,
        .sources = opts.sources,
        .c_flags = &.{},
    });

    const step = b.addInstallArtifact(exe, .{ .dest_dir = .{
        .override = .{ .custom = "../bin" },
    } });
    b.getInstallStep().dependOn(&step.step);
}

pub fn build(b: *std.Build) !void {

    // context
    const target64 = std.Build.resolveTargetQuery(b, .{
        .cpu_arch = std.Target.Cpu.Arch.x86_64,
        .os_tag = std.Target.Os.Tag.freestanding,
        .abi = std.Target.Abi.gnu,
        .ofmt = std.Target.ObjectFormat.elf,
    });
    const optimize = std.builtin.OptimizeMode.ReleaseFast;

    // kernel
    build_kern_binary(b, .{
        .name = "kernel",
        .target = target64,
        .optimize = optimize,
        .sources = &.{
            kernel_src,
        },
        .linker = "config/kernel.ld",
        .include = "kernel/include",
    });
}

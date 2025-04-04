//! Copyright (c) 2025 Freya Murphy <freya@freyacat.org>

const std = @import("std");
const builtin = @import("builtin");

const c_flags = &[_][]const u8{
    // lang
    "-std=c99",
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

const boot_src = &[_][]const u8{"boot/boot.S"};

const kernel_src = &[_][]const u8{
    "kernel/entry.S", // must be first
    "kernel/kernel.c", // main function
    "kernel/cpu/cpu.c",
    "kernel/cpu/fpu.c",
    "kernel/cpu/idt.c",
    "kernel/cpu/idt.S",
    "kernel/cpu/pic.c",
    "kernel/io/io.c",
    "kernel/io/panic.c",
    "kernel/mboot/mboot.c",
    "kernel/mboot/mmap.c",
    "kernel/memory/memory.c",
    "kernel/memory/paging.c",
    "kernel/memory/physalloc.c",
    "kernel/memory/virtalloc.c",
};

const lib_src = &[_][]const u8{
    "lib/alloc.c",
    "lib/atox.c",
    "lib/bound.c",
    "lib/btoa.c",
    "lib/ctoi.c",
    "lib/delay.c",
    "lib/isdigit.c",
    "lib/isspace.c",
    "lib/itoc.c",
    "lib/memcmp.c",
    "lib/memcpy.c",
    "lib/memcpyv.c",
    "lib/memmove.c",
    "lib/memmovev.c",
    "lib/memset.c",
    "lib/memsetv.c",
    "lib/printf.c",
    "lib/stpcpy.c",
    "lib/stpncpy.c",
    "lib/strcat.c",
    "lib/strcpy.c",
    "lib/strlen.c",
    "lib/strncmp.c",
    "lib/strncpy.c",
    "lib/strtoux.c",
    "lib/strtox.c",
    "lib/uxtoa.c",
    "lib/xtoa.c",
};

const Prog = struct {
    name: []const u8,
    source: []const []const u8,
};

const util_progs = &[_]Prog{
    // mkblob
    Prog{
        .name = "mkblob",
        .source = &[_][]const u8{"util/mkblob.c"},
    },
    // listblob
    Prog{
        .name = "listblob",
        .source = &[_][]const u8{"util/listblob.c"},
    },
    // BuildImage
    Prog{
        .name = "BuildImage",
        .source = &[_][]const u8{"util/BuildImage.c"},
    },
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

    // add include paths
    exe.addIncludePath(b.path("include/"));
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
    const target32 = std.Build.resolveTargetQuery(b, .{
        .cpu_arch = std.Target.Cpu.Arch.x86,
        .os_tag = std.Target.Os.Tag.freestanding,
        .abi = std.Target.Abi.gnu,
        .ofmt = std.Target.ObjectFormat.elf,
    });
    const target64 = std.Build.resolveTargetQuery(b, .{
        .cpu_arch = std.Target.Cpu.Arch.x86_64,
        .os_tag = std.Target.Os.Tag.freestanding,
        .abi = std.Target.Abi.gnu,
        .ofmt = std.Target.ObjectFormat.elf,
    });
    const optimize = std.builtin.OptimizeMode.ReleaseFast;

    // boot
    build_kern_binary(b, .{
        .name = "boot",
        .target = target32,
        .optimize = optimize,
        .sources = &.{
            boot_src,
        },
        .linker = "boot/boot.ld",
        .entry = "bootentry",
        .include = "boot/include",
    });

    // kernel
    build_kern_binary(b, .{
        .name = "kernel",
        .target = target64,
        .optimize = optimize,
        .sources = &.{
            kernel_src,
            lib_src,
        },
        .linker = "kernel/kernel.ld",
        .include = "kernel/include",
    });

    // util_progs
    for (util_progs) |prog| {
        build_native_binary(b, .{
            .name = prog.name,
            .optimize = optimize,
            .sources = &.{
                prog.source,
            },
        });
    }
}

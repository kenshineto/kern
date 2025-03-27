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

const ld_flags = &[_][]const u8{
    "-nmagic",
    "-nostdlib",
    "--no-warn-rwx-segments",
};

const boot_src = &[_][]const u8{"boot/boot.S"};

const kernel_src = &[_][]const u8{
    "kernel/cio.c",
    "kernel/clock.c",
    "kernel/isrs.S",
    "kernel/kernel.c",
    "kernel/kmem.c",
    "kernel/list.c",
    "kernel/procs.c",
    "kernel/sio.c",
    "kernel/startup.S",
    "kernel/support.c",
    "kernel/syscalls.c",
    "kernel/user.c",
    "kernel/vm.c",
    "kernel/vmtables.c",
    "lib/klibc.c",
};

const lib_src = &[_][]const u8{
    "lib/bound.c",
    "lib/cvtdec0.c",
    "lib/cvtdec.c",
    "lib/cvthex.c",
    "lib/cvtoct.c",
    "lib/cvtuns0.c",
    "lib/cvtuns.c",
    "lib/memclr.c",
    "lib/memcpy.c",
    "lib/memset.c",
    "lib/pad.c",
    "lib/padstr.c",
    "lib/sprint.c",
    "lib/str2int.c",
    "lib/strcat.c",
    "lib/strcmp.c",
    "lib/strcpy.c",
    "lib/strlen.c",
};

const ulib_src = &[_][]const u8{
    "lib/entry.S",
    "lib/ulibc.c",
    "lib/ulibs.S",
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

const user_progs = &[_]Prog{
    // idle
    Prog{
        .name = "idle",
        .source = &[_][]const u8{"user/idle.c"},
    },
    // init
    Prog{
        .name = "init",
        .source = &[_][]const u8{"user/init.c"},
    },
    // progABC
    Prog{
        .name = "progABC",
        .source = &[_][]const u8{"user/progABC.c"},
    },
    // progDE
    Prog{
        .name = "progDE",
        .source = &[_][]const u8{"user/progDE.c"},
    },
    // progFG
    Prog{
        .name = "progFG",
        .source = &[_][]const u8{"user/progFG.c"},
    },
    // progH
    Prog{
        .name = "progH",
        .source = &[_][]const u8{"user/progH.c"},
    },
    // progI
    Prog{
        .name = "progI",
        .source = &[_][]const u8{"user/progI.c"},
    },
    // progJ
    Prog{
        .name = "progJ",
        .source = &[_][]const u8{"user/progJ.c"},
    },
    // progKL
    Prog{
        .name = "progKL",
        .source = &[_][]const u8{"user/progKL.c"},
    },
    // progKL
    Prog{
        .name = "progKL",
        .source = &[_][]const u8{"user/progKL.c"},
    },
    // progMN
    Prog{
        .name = "progMN",
        .source = &[_][]const u8{"user/progMN.c"},
    },
    // progP
    Prog{
        .name = "progP",
        .source = &[_][]const u8{"user/progP.c"},
    },
    // progQ
    Prog{
        .name = "progQ",
        .source = &[_][]const u8{"user/progQ.c"},
    },
    // progR
    Prog{
        .name = "progR",
        .source = &[_][]const u8{"user/progR.c"},
    },
    // progS
    Prog{
        .name = "progS",
        .source = &[_][]const u8{"user/progS.c"},
    },
    // progTUV
    Prog{
        .name = "progTUV",
        .source = &[_][]const u8{"user/progTUV.c"},
    },
    // progW
    Prog{
        .name = "progW",
        .source = &[_][]const u8{"user/progW.c"},
    },
    // progX
    Prog{
        .name = "progX",
        .source = &[_][]const u8{"user/progX.c"},
    },
    // progY
    Prog{
        .name = "progY",
        .source = &[_][]const u8{"user/progY.c"},
    },
    // progZ
    Prog{
        .name = "progZ",
        .source = &[_][]const u8{"user/progZ.c"},
    },
    // shell
    Prog{
        .name = "shell",
        .source = &[_][]const u8{"user/shell.c"},
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
};

fn build_kern_binary(b: *std.Build, opts: BuildKernBinaryOpts) void {
    // create compile step
    const exe = b.addExecutable(.{
        .name = opts.name,
        .target = opts.target,
        .optimize = opts.optimize,
        .strip = true,
    });

    // add include path
    exe.addIncludePath(b.path("include/"));
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
    const target = b.standardTargetOptions(.{
        .default_target = .{
            .cpu_arch = std.Target.Cpu.Arch.x86,
            .os_tag = std.Target.Os.Tag.freestanding,
            .abi = std.Target.Abi.gnu,
            .ofmt = std.Target.ObjectFormat.elf,
        },
    });
    const optimize = std.builtin.OptimizeMode.ReleaseFast;

    // boot
    build_kern_binary(b, .{
        .name = "boot",
        .target = target,
        .optimize = optimize,
        .sources = &.{
            boot_src,
        },
        .linker = "boot/boot.ld",
        .entry = "bootentry",
    });

    // kernel
    build_kern_binary(b, .{
        .name = "kernel",
        .target = target,
        .optimize = optimize,
        .sources = &.{
            kernel_src,
            lib_src,
        },
        .linker = "kernel/kernel.ld",
    });

    // user_progs
    for (user_progs) |prog| {
        build_kern_binary(b, .{
            .name = prog.name,
            .target = target,
            .optimize = optimize,
            .sources = &.{
                prog.source,
                lib_src,
                ulib_src,
            },
            .linker = "user/user.ld",
        });
    }

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

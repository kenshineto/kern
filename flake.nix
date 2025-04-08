{
  description = "systems programming x86";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }: let
    supportedSystems = let
      inherit (flake-utils.lib) system;
    in [
      system.aarch64-linux
      system.aarch64-darwin
      system.x86_64-linux
    ];
  in
    flake-utils.lib.eachSystem supportedSystems (system: let
      pkgs = import nixpkgs {inherit system;};
    in {
      devShell =
        pkgs.mkShell
        {
          packages = with pkgs; [
            # (writeShellScriptBin "build" "zig build -Dcpu=baseline")
            gnumake
            gdb
            valgrind
            zig_0_14
            qemu
            grub2_light
            xorriso
          ];
        };

      formatter = pkgs.alejandra;
    });
}

{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    llvmPackages.clang-unwrapped
    llvmPackages.llvm
    lld
    nasm
    gnumake
    dosfstools
  ];
}

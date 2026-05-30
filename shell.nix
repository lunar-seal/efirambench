{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    llvmPackages.clang-unwrapped
    llvmPackages.clang-tools
    llvmPackages.llvm
    lld
    nasm
    gnumake
    dosfstools
    mtools
    xorriso
  ];
}

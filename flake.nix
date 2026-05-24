{
  description = "Small x86_64 UEFI RAM write-bandwidth benchmark";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      systems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              llvmPackages.clang-unwrapped
              llvmPackages.llvm
              lld
              nasm
              gnumake
              dosfstools
            ];
          };
        });
    };
}

# based on https://github.com/the-nix-way/dev-templates/blob/main/typst/flake.nix
{
    description = "esp32 c++ and python development environment";

    inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    outputs = { self, ... } @ inputs:
    let
        inherit (inputs.nixpkgs) lib;
        supportedSystems = [ "x86_64-linux" ];
        forEachSupportedSystem =
            f:
            inputs.nixpkgs.lib.genAttrs supportedSystems (
                system:
                f {
                    inherit system;
                    pkgs = import inputs.nixpkgs { inherit system; };
                }
            );
        version = "3.14";
    in {
        devShells = forEachSupportedSystem (
            { pkgs, system }:
            let
                concatMajorMinor = v: lib.pipe v [ lib.versions.splitVersion (lib.sublist 0 2) lib.concatStrings ];
                python = pkgs."python${concatMajorMinor version}";
                platformio = pkgs.buildFHSEnv {
                    name = "pio";
                    targetPkgs = pkgs: with pkgs; [
                        platformio-core
                        bash
                    ];
                    runScript = "${pkgs.platformio-core}/bin/pio";
                };
            in {
                default = pkgs.mkShell {
                    venvDir = ".venv";
                    shellHook = ''
                        export PLATFORMIO_CORE_DIR="$PWD/.platformio"
                        export PYTHONPATH="$PWD:$PYTHONPATH"

                        alias up="$PWD/tools/upload.sh complex"

                        upload() {
                            $PWD/tools/upload.sh $1
                        }
                    '';
                    packages = (with pkgs; [
                        platformio
                        clang-tools
                        cmake
                        cppcheck
                        libresprite
                    ]) ++ (with python.pkgs; [
                        venvShellHook
                        pip
                        pillow
                        numpy
                    ]);
                };
            }
        );
    };
}

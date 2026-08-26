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
                    name = "platformio-env";
                    targetPkgs = pkgs: with pkgs; [
                        platformio-core
                    ];
                };
            in {
                default = pkgs.mkShell {
                    venvDir = ".venv";
                    shellHook = ''
                        export PLATFORMIO_CORE_DIR="$PWD/.platformio"
                    '';
                    postShellHook = ''
                        venvVersionWarn() {
                      	    local venvVersion
                      	    venvVersion="$("$venvDir/bin/python" -c 'import platform; print(platform.python_version())')"

                      	    [[ "$venvVersion" == "${python.version}" ]] && return

                      	    echo "Warning: Python version mismatch: [$venvVersion (venv)] != [${python.version}]"
                            echo  "        Delete '$venvDir' and reload to rebuild for version ${python.version}"
                        }
                        venvVersionWarn

                        if [[ -z "$IN_PIO_FHS" && -z "$SKIP_PIO_FHS" ]]; then
                            export IN_PIO_FHS=1
                            exec ${platformio}/bin/platformio-env
                        fi
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

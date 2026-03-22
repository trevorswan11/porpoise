{
  description = "Porpoise language development.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    zig-flake.url = "github:mitchellh/zig-overlay";
    zls-flake = {
      url = "github:zigtools/zls?ref=0.15.1";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.zig-overlay.follows = "zig-flake";
    };
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      zig-flake,
      zls-flake,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [
            (final: prev: {
              zig = zig-flake.packages.${system}."0.15.2";
              zls = zls-flake.packages.${system}.default.overrideAttrs (old: {
                nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [ final.zig ];
              });
            })
          ];
        };
      in
      with pkgs;
      {
        devShells.default = mkShell {
          buildInputs = [
            zig
            zls
          ]
          ++ (with llvmPackages_21; [
            clang-tools
            lldb
          ]);

          shellHook = ''
            # Without this, Zig freaks out over unknown flags
            export NIX_CFLAGS_COMPILE=$(echo $NIX_CFLAGS_COMPILE | sed 's/-fmacro-prefix-map=[^ ]*//g')
            export NIX_LDFLAGS=$(echo $NIX_LDFLAGS | sed 's/-fmacro-prefix-map=[^ ]*//g')

            # Required for LLDB on macOS (stinky)
            ${lib.optionalString stdenv.isDarwin ''
              # Tested both paths on my machine and they both work (adds some flexibility)
              if [[ -z "$LLDB_DEBUGSERVER_PATH" ]]; then
                XCODE_PATH="/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Versions/A/Resources/debugserver"
                CLT_PATH="/Library/Developer/CommandLineTools/Library/PrivateFrameworks/LLDB.framework/Versions/A/Resources/debugserver"

                if [[ -f "$XCODE_PATH" ]]; then
                    export LLDB_DEBUGSERVER_PATH="$XCODE_PATH"
                elif [[ -f "$CLT_PATH" ]]; then
                    export LLDB_DEBUGSERVER_PATH="$CLT_PATH"
                fi
              fi
            ''}
          '';
        };
      }
    );
}

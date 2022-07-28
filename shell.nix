#...
{ pkgs ? import <nixpkgs> {} } :
  let
  in
    pkgs.mkShell {
      nativeBuildInputs = with pkgs.buildPackages; [ 
	git-lfs
	clang
	pkg-config
	gcc
        ffmpeg
      ];
    }

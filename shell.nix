#...
{ } :
  let
    # Pinned nixpkgs
    pkgs = import (builtins.fetchGit {
      name = "nixpkg-22.05";                         
      url = "https://github.com/NixOS/nixpkgs/";             
      ref = "refs/tags/22.05";           
      rev = "ce6aa13369b667ac2542593170993504932eb836";                       
    }) {};                                       
  in
    pkgs.mkShell {
      nativeBuildInputs = with pkgs.buildPackages; [ 
	git-lfs
	clang
	pkg-config
	gef
	gcc
        ffmpeg
      ];
    }

{
    pkgs ? import<nixpkgs>{ }
}:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    gcc
    rust
    xorriso
  ];
}


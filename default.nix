with import <nixpkgs> {};

stdenv.mkDerivation {
  pname = "trdrop";
  version = "latest";

  src = ./trdrop;

  preConfigure = ''
    sed -i 's;/usr/local/include/opencv4;${opencv4}/include/opencv4;g' trdrop.pro
    sed -i 's;/usr/local/lib;${opencv4}/lib;g' trdrop.pro
    sed -i 's;/opt/''$''${TARGET};/;g' trdrop.pro
  '';

  qmakeFlags = [
    "CONFIG+=qtquickcompiler"
  ];

  nativeBuildInputs = with qt5; [ qmake wrapQtAppsHook ];
  buildInputs = with qt5; [
    qtbase
    qtmultimedia
    qtquickcontrols2
  ];
  installPhase = ''
    mkdir -p $out/bin
    cp -rf trdrop $out/bin/
  '';
}

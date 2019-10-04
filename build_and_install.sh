#!/bin/bash

echo "build helper script for derperview"

if [[ -d build/ ]]; then
    echo "remove old build folder"
    rm -fr build
fi

echo "create new build folder"
mkdir build
cd build

echo "run cmake for out of source build"
if [[ ${EUID} -eq 0 ]]; then
    echo "script is run as root, start installation to local system bin dir"
    cmake ../ -DCMAKE_BUILD_TYPE=Release
else
    echo "script was not run as root, install to local bin dir"
    cmake ../  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../
fi

echo "running build"
cmake --build .

echo "installing built binaries"
make install


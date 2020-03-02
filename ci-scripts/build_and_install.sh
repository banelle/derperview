#!/bin/bash

echo "build helper script for derperview"

if [[ -d build/ ]]; then
    echo "remove old build folder"
    rm -fr build
fi

echo "create new build folder"
mkdir build
cd build

echo "running cmake with out of source build setup"
if [[ "${TRAVIS_OS_NAME}" == "windows" ]]; then
    # on travis windows builds ignore sh.exe
    cmake ../ -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../
    if [[ $? -ne 0 ]]; then
        echo "cmake configuration failed"
        cat CMakeFiles/CMakeError.log
        exit 1
    fi
else
    cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../
    if [[ $? -ne 0 ]]; then
        echo "cmake configuration failed"
        cat CMakeFiles/CMakeError.log
        exit 1
    fi
fi


if [[ "${TRAVIS_OS_NAME}" == "windows" ]]; then
    # create bin/Release folder on windows builds
    mkdir -p ../bin/Release
fi

echo "running build"
cmake --build .
if [[ $? -ne 0 ]]; then
    echo "build failed"
    exit 1
fi

echo "installing built binaries"
cmake --build . --target install
if [[ $? -ne 0 ]]; then
    echo "install failed"
    exit 1
fi


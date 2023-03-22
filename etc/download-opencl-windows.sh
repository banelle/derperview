#!/bin/sh

mkdir -p ../lib/opencl
pushd ../lib

curl -O -L https://github.com/KhronosGroup/OpenCL-SDK/releases/download/v2023.02.06/OpenCL-SDK-v2023.02.06-Win-x64.zip
unzip -o OpenCL-SDK-v2023.02.06-Win-x64.zip
mv OpenCL-SDK-v2023.02.06-Win-x64/lib/OpenCL.lib opencl
mv OpenCL-SDK-v2023.02.06-Win-x64/include/ opencl

popd

#!/bin/sh

WXWIDGETS_VERSION=3.2.2

mkdir -p ../lib/wxwidgets-windows/lib
mkdir -p ../lib/wxwidgets-windows/bin
pushd ../lib

curl -O -L https://github.com/wxWidgets/wxWidgets/releases/download/v${WXWIDGETS_VERSION}/wxWidgets-${WXWIDGETS_VERSION}-headers.7z
7z x -y wxWidgets-${WXWIDGETS_VERSION}-headers.7z
mv include wxwidgets-windows

curl -O -L https://github.com/wxWidgets/wxWidgets/releases/download/v${WXWIDGETS_VERSION}/wxMSW-${WXWIDGETS_VERSION}_vc14x_x64_Dev.7z
7z x -y wxMSW-${WXWIDGETS_VERSION}_vc14x_x64_Dev.7z
mv lib/vc14x_x64_dll/*.lib wxwidgets-windows/lib
mv lib/vc14x_x64_dll/*.dll wxwidgets-windows/bin
mv lib/vc14x_x64_dll/mswu wxwidgets-windows/include
mv lib/vc14x_x64_dll/mswud wxwidgets-windows/include

curl -O -L https://github.com/wxWidgets/wxWidgets/releases/download/v${WXWIDGETS_VERSION}/wxMSW-${WXWIDGETS_VERSION}_vc14x_x64_ReleaseDLL.7z
7z x -y wxMSW-${WXWIDGETS_VERSION}_vc14x_x64_ReleaseDLL.7z
mv lib/vc14x_x64_dll/*.dll wxwidgets-windows/bin

popd

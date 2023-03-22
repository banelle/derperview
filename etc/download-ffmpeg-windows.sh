#!/bin/sh

FFMPEG_VERSION=5.1.2

mkdir -p ../lib/ffmpeg-windows
pushd ../lib

curl -O https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-${FFMPEG_VERSION}-full_build-shared.7z
7z x -y ffmpeg-${FFMPEG_VERSION}-full_build-shared.7z
mv ffmpeg-${FFMPEG_VERSION}-full_build-shared/lib/ ffmpeg-windows
mv ffmpeg-${FFMPEG_VERSION}-full_build-shared/include/ ffmpeg-windows
mv ffmpeg-${FFMPEG_VERSION}-full_build-shared/bin/ ffmpeg-windows

popd

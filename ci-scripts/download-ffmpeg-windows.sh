#!/bin/sh

FFMPEG_VERSION=4.2.1

mkdir -p ../lib
pushd ../lib

curl -O https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-${FFMPEG_VERSION}-win64-dev.zip
unzip -o ffmpeg-${FFMPEG_VERSION}-win64-dev.zip
mv ffmpeg-${FFMPEG_VERSION}-win64-dev ffmpeg-dev-windows

curl -O https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-${FFMPEG_VERSION}-win64-shared.zip
unzip -o ffmpeg-${FFMPEG_VERSION}-win64-shared.zip
mv ffmpeg-${FFMPEG_VERSION}-win64-shared ffmpeg-shared-windows

popd

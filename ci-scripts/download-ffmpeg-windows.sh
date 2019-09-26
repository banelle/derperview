#!/bin/sh

FFMPEG_VERSION=4.2.1

wget https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-${FFMPEG_VERSION}-win64-dev.zip
7z x -y ffmpeg-${FFMPEG_VERSION}-win64-dev.zip
mv ffmpeg-${FFMPEG_VERSION}-win64-dev ffmpeg-dev-windows

wget https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-${FFMPEG_VERSION}-win64-shared.zip
7z x -y ffmpeg-${FFMPEG_VERSION}-win64-shared.zip
mv ffmpeg-${FFMPEG_VERSION}-win64-shared ffmpeg-shared-windows


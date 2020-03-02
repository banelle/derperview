#!/bin/sh

pushd ../bin/Release
cp ../../lib/ffmpeg-shared-windows/bin/swresample-3.dll ./
cp ../../lib/ffmpeg-shared-windows/bin/avcodec-58.dll ./
cp ../../lib/ffmpeg-shared-windows/bin/avformat-58.dll ./
cp ../../lib/ffmpeg-shared-windows/bin/avutil-56.dll ./
7z a ../../deploy/derperview-win64.zip derperview.exe swresample-3.dll avcodec-58.dll avformat-58.dll avutil-56.dll
popd

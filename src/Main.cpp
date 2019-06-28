#include <cstring>
#include <sstream>
#include <chrono>
#include <cmath>
#include <thread>
#include "Process.hpp"
#include "Video.hpp"

using namespace std;
using namespace DerperView;

const int TOTAL_THREADS = 4;

void Go(string filename)
{
    InputVideoFile input(filename);
    input.Dump();
    
    auto videoInfo = input.GetVideoInfo();
    videoInfo.width = videoInfo.width * 4 / 3;
    videoInfo.bitRate = static_cast<int>(videoInfo.bitRate * 1.4);
    OutputVideoFile output(filename + ".out.mp4", videoInfo);

    int64_t percentageMarker = static_cast<int64_t>(floor(static_cast<float>(videoInfo.totalFrames) / 100));
    int64_t frameCount = 0;
    int64_t encodedFrameCount = 0;
    int frameBufferSize = -1;

    AVFrame *outputFrame = nullptr;
    vector<unsigned char> data[TOTAL_THREADS];
    vector<unsigned char> derperviewedData[TOTAL_THREADS];
    int targetWidth = 0;
    int pixelFormat = -1;
    int threadIndex = -1;
    thread threads[TOTAL_THREADS];
    
    cout << "--------------------------------------------------------------------" <<  endl;

    auto frame = input.GetNextFrame();
    while (frame != nullptr)
    {
        if (frame->width == 0) // Audio - stream it through
        {
            output.WriteNextFrame(frame);
        }
        else // Video, stretch that bad boy.
        {
            // If this is the first video frame we've seen, set our buffers and stuff up
            if (threadIndex == -1)
            {
                pixelFormat = frame->format;

                if (frame->width * 3 / 4 != frame->height) // Funky looking maths to keep this int
                {
                    cerr << "Source not in 4:3 aspect ratio" << endl;
                    return;
                }

                frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
                targetWidth = frame->width * 4 / 3;
                for (int i = 0; i < TOTAL_THREADS; i++)
                {
                    data[i].resize(frameBufferSize);
                    derperviewedData[i].resize(static_cast<int>(frame->height * targetWidth * 1.5));
                }

                threadIndex = 0;
            }

            // Copy data into a contiguous buffer we can mess with
            auto copyResult = av_image_copy_to_buffer(data[threadIndex].data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

            // Perform the stretchy stuff
            threads[threadIndex] = thread(ProcessFrameYuv, frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
            threadIndex ++;

            if (threadIndex >= TOTAL_THREADS)
            {
                for (int i = 0; i < TOTAL_THREADS; i ++)
                    threads[i].join();

                for (int i = 0; i < TOTAL_THREADS; i++)
                {
                    // Put new data into an AVFrame
                    outputFrame = av_frame_alloc();
                    outputFrame->width = targetWidth;
                    outputFrame->height = frame->height;
                    outputFrame->format = frame->format;
                    outputFrame->pts = frameCount;
                    av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

                    encodedFrameCount += output.WriteNextFrame(outputFrame);

                    av_frame_free(&outputFrame);
                    frameCount++;

                    if (frameCount % percentageMarker == 0)
                        cout << " " << ceil(static_cast<float>(frameCount) * 100 / videoInfo.totalFrames) << "% ";
                    else if (frameCount % 5 == 0)
                        cout << ".";
                }

                threadIndex = 0;
            }
        }

        frame = input.GetNextFrame();
    }

    // Clear left-over frames out of the thread buffer
    for (int i = 0; i < threadIndex; i++)
        threads[i].join();

    for (int i = 0; i < threadIndex; i++)
    {
        outputFrame = av_frame_alloc();
        outputFrame->width = targetWidth;
        outputFrame->height = videoInfo.height;
        outputFrame->format = pixelFormat;
        outputFrame->pts = frameCount;
        av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

        encodedFrameCount += output.WriteNextFrame(outputFrame);

        av_frame_free(&outputFrame);
        frameCount++;

        if (frameCount % percentageMarker == 0)
            cout << " " << ceil(static_cast<float>(frameCount) * 100 / videoInfo.totalFrames) << "% ";
        else if (frameCount % 5 == 0)
            cout << ".";
    }

    output.Flush();

    cout << endl;
    cout << "Encoded packet count:" << encodedFrameCount << endl;
    cout << "Frames read: " << frameCount << endl;

    cout << "--------------------------------------------------------------------" << endl;
}

int main(int argc, char **argv)
{
    chrono::system_clock clock;
    auto startTime = clock.now();
    string filename(argv[1]);

    Go(filename);

    cout << "--------------------------------------------------------------------" << endl;
    
    auto endTime = clock.now();
    auto minutes = chrono::duration_cast<chrono::minutes>(endTime - startTime).count();
    auto seconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count() % 60;
    cout << "Total time: " << minutes << "m " << seconds << "s" << endl;

    return 0;
}

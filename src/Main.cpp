#include <cstring>
#include <sstream>
#include <chrono>
#include <cmath>
#include "Process.hpp"
#include "Video.hpp"

using namespace std;
using namespace DerperView;

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

    vector<unsigned char> data;
    vector<unsigned char> derperviewedData;
    int targetWidth = 0;

    cout << "--------------------------------------------------------------------" <<  endl;

    auto frame = input.GetNextFrame();
    while (frame != nullptr)
    {
        AVFrame *outputFrame = nullptr;
        if (frame->width == 0) // Audio - stream it through
        {
            output.WriteNextFrame(frame);
        }
        else // Video, stretch that bad boy.
        {
            // If this is the first video frame we've seen, set our buffers and stuff up
            if (data.size() == 0)
            {
                if (frame->width * 3 / 4 != frame->height) // Funky looking maths to keep this int
                {
                    cerr << "Source not in 4:3 aspect ratio" << endl;
                    return;
                }
                if (frame != nullptr)
                {
                    frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
                    data.resize(frameBufferSize);
                    targetWidth = frame->width * 4 / 3;
                    derperviewedData.resize(static_cast<int>(frame->height * targetWidth * 1.5));
                }
            }

            // Copy data into a contiguous buffer we can mess with
            auto copyResult = av_image_copy_to_buffer(data.data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

            // Perform the stretchy stuff
            ProcessFrameYuv(frame->width, frame->height, data, derperviewedData);

            // Put new data into an AVFrame
            outputFrame = av_frame_alloc();
            outputFrame->width = targetWidth;
            outputFrame->height = frame->height;
            outputFrame->format = frame->format;
            outputFrame->pts = frame->pts;
            av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData.data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

            encodedFrameCount += output.WriteNextFrame(outputFrame);

            av_frame_free(&outputFrame);
            frameCount++;

            if (frameCount % percentageMarker == 0)
                cout << " " << ceil(static_cast<float>(frameCount) * 100 / videoInfo.totalFrames) << "% ";
            else if (frameCount % 5 == 0)
                cout << ".";
        }

        frame = input.GetNextFrame();
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

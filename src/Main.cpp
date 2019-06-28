#include <cstring>
#include <sstream>
#include "Process.hpp"
#include "Video.hpp"

using namespace std;
using namespace DerperView;

int main(int argc, char **argv)
{
    string filename(argv[1]);

    //DerperView::Test();

    InputVideoFile input(filename);
    input.Dump();
    
    auto videoInfo = input.GetVideoInfo();
    videoInfo.width = videoInfo.width * 4 / 3;
    videoInfo.bitRate = static_cast<int>(videoInfo.bitRate * 1.4);
    OutputVideoFile output(filename + ".out.mp4", videoInfo);

    int frameIndex = -1;
    int frameCount = 0;
    int encodedFrameCount = 0;
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
            // Copy data into a contiguous buffer we can mess with
            auto frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
            vector<unsigned char> data(frameBufferSize);
            auto copyResult = av_image_copy_to_buffer(data.data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

            // Perform the stretchy stuff
            vector<unsigned char> derperviewedData;
            int newWidth = ProcessFrameYuv(frame->width, frame->height, data, derperviewedData);

            // Put new data into an AVFrame
            outputFrame = av_frame_alloc();
            outputFrame->width = newWidth;
            outputFrame->height = frame->height;
            outputFrame->format = frame->format;
            outputFrame->pts = frame->pts;
            av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData.data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

            encodedFrameCount += output.WriteNextFrame(outputFrame);

            av_frame_free(&outputFrame);
            frameCount++;
        }

        cout << ".";

        frame = input.GetNextFrame();
    }

    output.Flush();

    cout << endl;
    cout << "Encoded packet count:" << encodedFrameCount << endl;
    cout << "Frames read: " << frameCount << endl;

    return 0;
}

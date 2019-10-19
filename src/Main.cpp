#include <cstring>
#include <sstream>
#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>
#include "Process.hpp"
#include "Video.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

using namespace std;
using namespace DerperView;

cxxopts::Options options("derperview", "creates derperview (like superview) from 4:3 videos");

int Go(const string inputFilename, const string outputFilename, const int totalThreads)
{
    InputVideoFile input(inputFilename);
    if (input.GetLastError() != 0)
        return input.GetLastError();
    input.Dump();
    
    auto inputVideoInfo = input.GetVideoInfo();

    if (inputVideoInfo.width * 3 / 4 != inputVideoInfo.height)
    {
        cerr << "Source not in 4:3 aspect ratio" << endl;
        return 1;
    }
    if (
        inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUV420P
        && inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUVJ420P
    )
    {
        cerr << "Source not in compatible pixel format" << endl;
        return 2;
    }

    auto outputVideoInfo = inputVideoInfo; // Copy video info and tweak for output
    outputVideoInfo.width = GetDerpedWidth(inputVideoInfo.width);
    outputVideoInfo.bitRate = static_cast<int>(inputVideoInfo.bitRate * 1.4);
    OutputVideoFile output(outputFilename, outputVideoInfo);
    if (output.GetLastError() != 0)
        return output.GetLastError();

    int64_t percentageMarker = static_cast<int64_t>(floor(static_cast<float>(inputVideoInfo.totalFrames) / 100));
    int64_t frameCount = 0;
    int64_t encodedPacketCount = 0;

    AVFrame *outputFrame = nullptr;
    vector<vector<unsigned char>> data(totalThreads);
    vector<vector<unsigned char>> derperviewedData(totalThreads);
    vector<thread> threads(totalThreads);
    int threadIndex = 0;

    // Allocate buffers
    auto frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(inputVideoInfo.pixelFormat), inputVideoInfo.width, inputVideoInfo.height, 1);
    auto derpBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(outputVideoInfo.pixelFormat), outputVideoInfo.width, outputVideoInfo.height, 1);
    for (int i = 0; i < totalThreads; i++)
    {
        data[i].resize(frameBufferSize);
        derperviewedData[i].resize(derpBufferSize);
    }
    
    cout << "Running up with " << totalThreads << " thread" << (totalThreads > 1 ? "s" : "") << "..." << endl;
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
            // Copy data into a contiguous buffer we can mess with
            auto copyResult = av_image_copy_to_buffer(data[threadIndex].data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

            // Set up thread to perform the stretchy stuff
            if (inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUV420P || inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUVJ420P)
                threads[threadIndex] = thread(ProcessFrameYuv, frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
            threadIndex ++;

            // If we've got all of our threads, then join the lot and write them to the output
            if (threadIndex >= totalThreads)
            {
                for (int i = 0; i < totalThreads; i ++)
                    threads[i].join();

                for (int i = 0; i < totalThreads; i++)
                {
                    // Put new data into an AVFrame
                    outputFrame = av_frame_alloc();
                    outputFrame->width = outputVideoInfo.width;
                    outputFrame->height = outputVideoInfo.height;
                    outputFrame->format = outputVideoInfo.pixelFormat;
                    outputFrame->pts = frameCount;
                    av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

                    encodedPacketCount += output.WriteNextFrame(outputFrame);

                    av_frame_free(&outputFrame);
                    frameCount++;

                    if (frameCount % percentageMarker == 0)
                    {
                        cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
                        cout.flush();
                    }
                    else if (frameCount % 5 == 0)
                    {
                        cout << ".";
                        cout.flush();
                    }
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
        outputFrame->width = outputVideoInfo.width;
        outputFrame->height = outputVideoInfo.height;
        outputFrame->format = outputVideoInfo.pixelFormat;
        outputFrame->pts = frameCount;
        av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

        encodedPacketCount += output.WriteNextFrame(outputFrame);

        av_frame_free(&outputFrame);
        frameCount++;

        if (frameCount % percentageMarker == 0)
            cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
        else if (frameCount % 5 == 0)
            cout << ".";
    }

    output.Flush();

    cout << endl;
    cout << "Encoded packet count: " << encodedPacketCount << endl;
    cout << "Frames read: " << frameCount << endl;

    cout << "--------------------------------------------------------------------" << endl;

    return 0;
}

void SuppressLibAvOutput(void *careface, int whatevs, const char *pfff, va_list sigh)
{
    // STFU
}

int main(int argc, char **argv)
{
    cout << "derperview " << VERSION << endl << endl;

    options.add_options()
        ("i,input", "Input filename", cxxopts::value<std::string>())
        ("o,output", "Output filename (default: INPUT_FILE + .out.mp4)", cxxopts::value<std::string>())
        ("q,stfu", "Suppress libav output", cxxopts::value<bool>()->default_value("false"))
        ("t,threads", "Process using given number of threads (default: 4)", cxxopts::value<unsigned int>())
        ("h,help", "Print help")
    ;

    options.parse_positional({"input"});
    auto args = options.parse(argc, argv);

    if (args.count("help"))
    {
      cout << options.help() << endl;
      exit(0);
    }

    string inputFilename;
    string outputFilename;
    unsigned int totalThreads = 4;

    if (args.count("input"))
    {
        inputFilename = args["input"].as<string>();
        cout << "using input filename: " << inputFilename << endl;
    }
    else
    {
        cerr << "no input filename given" << endl;
        exit(1);
    }

    if (args.count("output"))
    {
        outputFilename = args["output"].as<string>();
        cout << "using output filename: " << outputFilename << " (from command line)" << endl;
    }
    else
    {
        outputFilename = inputFilename + ".out.mp4";
        cout << "using output filename: " << outputFilename << " (derived from input filename)" << endl;
    }

    if (args.count("threads"))
    {
        totalThreads = args["threads"].as<unsigned int>();
        cout << "number of threads: " << totalThreads << " (from command line)" << endl;
    }
    else
        cout << "number of threads: " << totalThreads << " (default value)" << endl;

    if (args.count("stfu") && args["stfu"].as<bool>() == true)
    {
        av_log_set_callback(&SuppressLibAvOutput);
        cout << "setting shut up mode" << endl;
    }

    chrono::system_clock clock;
    auto startTime = clock.now();

    int result = Go(inputFilename, outputFilename, totalThreads);

    auto endTime = clock.now();
    auto minutes = chrono::duration_cast<chrono::minutes>(endTime - startTime).count();
    auto seconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count() % 60;
    cout << "Total time: " << minutes << "m " << seconds << "s" << endl;

    return result;

}


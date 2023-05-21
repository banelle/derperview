extern "C"
{
    #define __STDC_CONSTANT_MACROS
    #include "libavutil/common.h"
}

#include <cstring>
#include <sstream>
#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>
#include "libderperview.hpp"
#include "version.hpp"
#include "cxxopts.hpp"

using namespace std;

cxxopts::Options options("derperview", "creates derperview (like superview) from 4:3 videos");

void SuppressLibAvOutput(void* careface, int whatevs, const char* pfff, va_list sigh)
{
    // STFU
}

int main(int argc, char** argv)
{
    cout << "derperview " << VERSION << endl << endl;

    options.add_options()
        ("i,input", "Input filename", cxxopts::value<std::string>())
        ("o,output", "Output filename (default: INPUT_FILE + .out.mp4)", cxxopts::value<std::string>())
        ("q,stfu", "Suppress libav output", cxxopts::value<bool>()->default_value("false"))
        ("t,threads", "Process using given number of threads (default: 4)", cxxopts::value<unsigned int>())
        ("h,help", "Print help")
        ;

    options.parse_positional({ "input" });
    options.positional_help("INPUT_FILE");
    auto args = options.parse(argc, argv);

    if (args.count("help") || !args.count("input"))
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

    int result = Go(inputFilename, outputFilename, totalThreads, cout);

    auto endTime = clock.now();
    auto minutes = chrono::duration_cast<chrono::minutes>(endTime - startTime).count();
    auto seconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count() % 60;
    cout << "Total time: " << minutes << "m " << seconds << "s" << endl;

    return result;
}

#include "GpuProcess.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace DerperView;

string KERNEL_SOURCE =
"void kernel derperview("
"    const unsigned int sourceWidth,"
"    const unsigned int uOffsetSource,"
"    const unsigned int vOffsetSource,"
"    const unsigned int uOffsetTarget,"
"    const unsigned int vOffsetTarget,"
"    global const float* lookup,"
"    global const unsigned char* inData,"
"    global unsigned char* outData"
")"
"{"
"    int x = get_global_id(0);"
"    int y = get_global_id(1);"
"    int targetWidth = get_global_size(0);"
"    "
"    float value = lookup[x];"
"    float a0 = floor(value);"
"    float a1 = min(ceil(value), convert_float(sourceWidth - 1));"
"    int offset0 = convert_int(a0);"
"    int offset1 = convert_int(a1);"
""
"    float y0 = convert_float(inData[y * sourceWidth + offset0]);"
"    float y1 = convert_float(inData[y * sourceWidth + offset1]);"
"    float yout = mix(y0, y1, value - a0);"
"    outData[y * targetWidth + x] = convert_uchar(yout);"
""
"    if (y % 2 == 0 && x % 2 == 0)"
"    {"
"        float uvValue = lookup[x + 1];"
"        float uva0 = floor(uvValue);"
"        float uva1 = min(ceil(uvValue), convert_float(sourceWidth - 1));"
"        int uvOffset0 = convert_int(uva0);"
"        int uvOffset1 = convert_int(uva1);"
""
"        float u0 = convert_float(inData[uOffsetSource + (y * sourceWidth / 4 + uvOffset0 / 2)]);"
"        float u1 = convert_float(inData[uOffsetSource + (y * sourceWidth / 4 + uvOffset1 / 2)]);"
"        float v0 = convert_float(inData[vOffsetSource + (y * sourceWidth / 4 + uvOffset0 / 2)]);"
"        float v1 = convert_float(inData[vOffsetSource + (y * sourceWidth / 4 + uvOffset1 / 2)]);"
""
"        float uout = mix(u0, u1, uvValue - uva0);"
"        float vout = mix(v0, v1, uvValue - uva0);"
""
"        outData[uOffsetTarget + (y * targetWidth / 4 + x / 2)] = convert_uchar(uout);"
"        outData[vOffsetTarget + (y * targetWidth / 4 + x / 2)] = convert_uchar(vout);"
"    }"
"}"
;

GpuProcess::GpuProcess(unsigned int width, unsigned int height) : Process(width, height)
{
    auto platform = cl::Platform::getDefault();
    cout << "Platform Name: " << platform.getInfo<CL_PLATFORM_NAME>() << endl;
    cout << "Platform Vendor: " << platform.getInfo<CL_PLATFORM_VENDOR>() << endl;
    cout << "Platform Version: " << platform.getInfo<CL_PLATFORM_NUMERIC_VERSION>() << endl;

    cl::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    for (auto device : devices)
    {
        cout << "\tDevice Name: " << device.getInfo<CL_DEVICE_NAME>() << endl;
        cout << "\tDevice Type: " << device.getInfo<CL_DEVICE_TYPE>();
        cout << " (GPU: " << CL_DEVICE_TYPE_GPU << ", CPU: " << CL_DEVICE_TYPE_CPU << ")" << endl;
        cout << "\tDevice Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << endl;
        cout << "\tDevice Max Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl;
        cout << "\tDevice Global Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << endl;
        cout << "\tDevice Max Clock Frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << endl;
        cout << "\tDevice Max Allocateable Memory: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << endl;
        cout << "\tDevice Local Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << endl;
        cout << "\tDevice Available: " << device.getInfo< CL_DEVICE_AVAILABLE>() << endl << endl;
    }

    device_ = devices.front();

    context_ = cl::Context(device_);

    cl::Program program = cl::Program(context_, KERNEL_SOURCE);
    auto buildError = program.build();

    if (buildError != CL_BUILD_SUCCESS) {
        cerr << "Error!\nBuild Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device_)
            << "\nBuild Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device_) << std::endl;
        exit(1);
    }

    kernel_ = cl::Kernel(program, "derperview");

    lookupBuffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY, lookup_.size() * sizeof(float));
    inputBuffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY, static_cast<unsigned int>(height_ * sourceWidth_ * 1.5));
    outputBuffer_ = cl::Buffer(context_, CL_MEM_WRITE_ONLY, static_cast<unsigned int>(height_ * targetWidth_ * 1.5));

    const unsigned int uOffsetSource = height_ * sourceWidth_;
    const unsigned int vOffsetSource = static_cast<unsigned int>(height_ * sourceWidth_ * 1.25);
    const unsigned int uOffsetTarget = height_ * targetWidth_;
    const unsigned int vOffsetTarget = static_cast<unsigned int>(height_ * targetWidth_ * 1.25);

    kernel_.setArg(0, sourceWidth_);
    kernel_.setArg(1, uOffsetSource);
    kernel_.setArg(2, vOffsetSource);
    kernel_.setArg(3, uOffsetTarget);
    kernel_.setArg(4, vOffsetTarget);
    kernel_.setArg(5, lookupBuffer_);
    kernel_.setArg(6, inputBuffer_);
    kernel_.setArg(7, outputBuffer_);

    queue_ = cl::CommandQueue(context_, device_);

    queue_.enqueueWriteBuffer(lookupBuffer_, CL_TRUE, 0, lookup_.size() * sizeof(float), lookup_.data());
}

int GpuProcess::DerpIt(vector<unsigned char>& inData, vector<unsigned char>& outData)
{
    queue_.enqueueWriteBuffer(inputBuffer_, CL_TRUE, 0, inData.size(), inData.data());

    queue_.enqueueNDRangeKernel(kernel_, cl::NullRange, cl::NDRange(targetWidth_, height_), cl::NullRange);
    queue_.finish();

    queue_.enqueueReadBuffer(outputBuffer_, CL_TRUE, 0, outData.size(), outData.data());
    
    return 0;
}
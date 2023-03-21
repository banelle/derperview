#include "Process.hpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;
using namespace DerperView;

template <typename T>
T Lerp(T a, T b, double t)
{
    double ax = static_cast<double>(a);
    double bx = static_cast<double>(b);
    double result = ax + t * (bx - ax);
    return static_cast<T>(result);
}

int Process::GetDerpedWidth(int sourceWidth)
{
    int targetWidth = sourceWidth * 4 / 3;
    if (targetWidth % 2 == 1) // Since x264 won't encode video with an odd number of pixels in a row
        targetWidth -= 1;
    return targetWidth;
}

CpuProcess::CpuProcess(int width, int height) : Process(width, height)
{
    // Generate lookup table
    int targetWidth = width * 4 / 3;
    lookup_ = vector<double>(targetWidth);
    for (int tx = 0; tx < targetWidth; tx++)
    {
        double x = (static_cast<double>(tx) / targetWidth - 0.5) * 2; //  - 1 -> 1
        double sx = tx - (targetWidth - width) / 2;
        double offset = pow(x, 2) * (x < 0 ? -1 : 1) * ((targetWidth - width) / 2);
        lookup_[tx] = sx - offset;
    }
}

int CpuProcess::DerpIt(vector<unsigned char> &inData, vector<unsigned char> &outData)
{
    int targetWidth = GetDerpedWidth(width_);

    const int uOffsetSource = height_ * width_;
    const int vOffsetSource = static_cast<int>(height_ * width_ * 1.25);
    const int uOffsetTarget = height_ * targetWidth;
    const int vOffsetTarget = static_cast<int>(height_ * targetWidth * 1.25);

    for (int y = 0; y < height_; y ++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            auto value = lookup_[x];
            auto a0 = floor(value);
            auto a1 = min(ceil(value), static_cast<double>(width_ - 1));
            auto offset0 = static_cast<int>(a0);
            auto offset1 = static_cast<int>(a1);

            unsigned char y0 = inData[(y * width_ + offset0)];
            unsigned char y1 = inData[(y * width_ + offset1)];
            unsigned char yout = Lerp(y0, y1, value - a0);
            outData[(y * targetWidth + x)] = yout;

            if (y % 2 == 0 && x % 2 == 0)
            {
                auto uvValue = lookup_[x + 1];
                auto uva0 = floor(uvValue);
                auto uva1 = min(ceil(uvValue), static_cast<double>(width_ - 1));
                auto uvOffset0 = static_cast<int>(uva0);
                auto uvOffset1 = static_cast<int>(uva1);

                unsigned char u0 = inData[uOffsetSource + (y * width_ / 4 + uvOffset0 / 2)];
                unsigned char u1 = inData[uOffsetSource + (y * width_ / 4 + uvOffset1 / 2)];
                unsigned char v0 = inData[vOffsetSource + (y * width_ / 4 + uvOffset0 / 2)];
                unsigned char v1 = inData[vOffsetSource + (y * width_ / 4 + uvOffset1 / 2)];

                unsigned char uout = Lerp(u0, u1, uvValue - uva0);
                unsigned char vout = Lerp(v0, v1, uvValue - uva0);

                outData[uOffsetTarget + (y * targetWidth / 4 + x / 2)] = uout;
                outData[vOffsetTarget + (y * targetWidth / 4 + x / 2)] = vout;
            }
        }
    }

    return targetWidth;
}

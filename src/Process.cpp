#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;

template <typename T>
T Lerp(T a, T b, double t)
{
    double ax = static_cast<double>(a);
    double bx = static_cast<double>(b);
    double result = ax + t * (bx - ax);
    return static_cast<T>(result);
}

unordered_map<int, vector<double>> LOOKUP;

void BuildLookup(int width)
{
    // Generate lookup table
    int targetWidth = width * 4 / 3;
    LOOKUP[width] = vector<double>(targetWidth);
    for (int tx = 0; tx < targetWidth; tx++)
    {
        double x = (static_cast<double>(tx) / targetWidth - 0.5) * 2; //  - 1 -> 1
        double sx = tx - (targetWidth - width) / 2;
        double offset = pow(x, 2) * (x < 0 ? -1 : 1) * ((targetWidth - width) / 2);
        LOOKUP[width][tx] = sx - offset;
    }
}

int GetDerpedWidth(int sourceWidth)
{
    int targetWidth = sourceWidth * 4 / 3;
    if (targetWidth % 2 == 1) // Since x264 won't encode video with an odd number of pixels in a row
        targetWidth -= 1;
    return targetWidth;
}

int ProcessFrame(int width, int height, vector<unsigned char> &inData, vector<unsigned char> &outData)
{
    int targetWidth = GetDerpedWidth(width);

    // Have we done this size before?
    if (LOOKUP.find(width) == LOOKUP.end())
        BuildLookup(width);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            auto value = LOOKUP[width][x];
            auto a0 = floor(value);
            auto a1 = min(ceil(value), static_cast<double>(width - 1));
            auto offset0 = static_cast<int>(a0);
            auto offset1 = static_cast<int>(a1);

            unsigned char r0 = inData[(y * width + offset0) * 3 + 0];
            unsigned char g0 = inData[(y * width + offset0) * 3 + 1];
            unsigned char b0 = inData[(y * width + offset0) * 3 + 2];
            unsigned char r1 = inData[(y * width + offset1) * 3 + 0];
            unsigned char g1 = inData[(y * width + offset1) * 3 + 1];
            unsigned char b1 = inData[(y * width + offset1) * 3 + 2];

            unsigned char rout = Lerp(r0, r1, value - a0);
            unsigned char gout = Lerp(g0, g1, value - a0);
            unsigned char bout = Lerp(b0, b1, value - a0);

            outData[(y * targetWidth + x) * 3 + 0] = rout;
            outData[(y * targetWidth + x) * 3 + 1] = gout;
            outData[(y * targetWidth + x) * 3 + 2] = bout;
        }
    }

    return targetWidth;
}

int ProcessFrameYuv(int width, int height, std::vector<unsigned char> &inData, std::vector<unsigned char> &outData)
{
    int targetWidth = GetDerpedWidth(width);

    // Have we done this size before?
    if (LOOKUP.find(width) == LOOKUP.end())
        BuildLookup(width);

    const int uOffsetSource = height * width;
    const int vOffsetSource = static_cast<int>(height * width * 1.25);
    const int uOffsetTarget = height * targetWidth;
    const int vOffsetTarget = static_cast<int>(height * targetWidth * 1.25);

    for (int y = 0; y < height; y ++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            auto value = LOOKUP[width][x];
            auto a0 = floor(value);
            auto a1 = min(ceil(value), static_cast<double>(width - 1));
            auto offset0 = static_cast<int>(a0);
            auto offset1 = static_cast<int>(a1);

            unsigned char y0 = inData[(y * width + offset0)];
            unsigned char y1 = inData[(y * width + offset1)];
            unsigned char yout = Lerp(y0, y1, value - a0);
            outData[(y * targetWidth + x)] = yout;

            if (y % 2 == 0 && x % 2 == 0)
            {
                auto uvValue = LOOKUP[width][x + 1];
                auto uva0 = floor(uvValue);
                auto uva1 = min(ceil(uvValue), static_cast<double>(width - 1));
                auto uvOffset0 = static_cast<int>(uva0);
                auto uvOffset1 = static_cast<int>(uva1);

                unsigned char u0 = inData[uOffsetSource + (y * width / 4 + uvOffset0 / 2)];
                unsigned char u1 = inData[uOffsetSource + (y * width / 4 + uvOffset1 / 2)];
                unsigned char v0 = inData[vOffsetSource + (y * width / 4 + uvOffset0 / 2)];
                unsigned char v1 = inData[vOffsetSource + (y * width / 4 + uvOffset1 / 2)];

                unsigned char uout = Lerp(u0, u1, uvValue - uva0);
                unsigned char vout = Lerp(v0, v1, uvValue - uva0);

                outData[uOffsetTarget + (y * targetWidth / 4 + x / 2)] = uout;
                outData[vOffsetTarget + (y * targetWidth / 4 + x / 2)] = vout;
            }
        }
    }

    return targetWidth;
}

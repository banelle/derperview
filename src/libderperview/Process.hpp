#pragma once

#include <vector>

namespace DerperView
{
    class Process
    {
    public:
        Process(unsigned int width, unsigned int height);

        virtual ~Process() { };

        virtual int DerpIt(std::vector<unsigned char>& inData, std::vector<unsigned char>& outData) = 0;
        static int GetDerpedWidth(int sourceWidth);

    protected:
        unsigned int sourceWidth_;
        unsigned int targetWidth_;
        unsigned int height_;
        std::vector<float> lookup_;
    };
    
    class CpuProcess : public Process
    {
    public:
        CpuProcess(unsigned int width, unsigned int height) : Process(width, height) { }

        virtual int DerpIt(std::vector<unsigned char>& inData, std::vector<unsigned char>& outData) override;
    };
}

#include <vector>
#include <CL/opencl.hpp>
#include "Process.hpp"

namespace DerperView
{
    class GpuProcess : public Process
    {
    public:
        GpuProcess(unsigned int width, unsigned int height);

        virtual int DerpIt(std::vector<unsigned char>& inData, std::vector<unsigned char>& outData) override;

    private:
        cl::Device device_;
        cl::Context context_;
        cl::Kernel kernel_;
        cl::Buffer lookupBuffer_;
        cl::Buffer inputBuffer_;
        cl::Buffer outputBuffer_;
        cl::CommandQueue queue_;
    };
}

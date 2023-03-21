#include <vector>

namespace DerperView
{
    class Process
    {
    public:
        Process(int width, int height) : width_(width), height_(height) { }
        virtual ~Process() { };

        virtual int DerpIt(std::vector<unsigned char>& inData, std::vector<unsigned char>& outData) = 0;
        static int GetDerpedWidth(int sourceWidth);

    protected:
        int width_;
        int height_;
    };
    
    class CpuProcess : public Process
    {
    public:
        CpuProcess(int width, int height);

        virtual int DerpIt(std::vector<unsigned char>& inData, std::vector<unsigned char>& outData) override;

    private:
        std::vector<double> lookup_;
    };
}

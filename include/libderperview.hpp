#include <string>
#include <iostream>
#include <functional>

int Go(const std::string inputFilename, const std::string outputFilename, const int totalThreads, std::ostream& outputStream, std::function<void(int)> callback = nullptr, const bool& cancel = false);

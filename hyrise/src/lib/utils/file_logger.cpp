#include "file_logger.hpp"
#include <filesystem>
#include <mutex>

namespace hyrise
{

std::mutex fileMutex;

void log_to_file(std::string s)
{

    std::lock_guard<std::mutex> lock(fileMutex);
    std::string filename = "log_" + std::to_string(getpid()) + ".txt";
    std::filesystem::path filePath(filename);
    std::ofstream outputFile(filePath, std::ios::app);
    outputFile << s;
}
} // namespace hyrise

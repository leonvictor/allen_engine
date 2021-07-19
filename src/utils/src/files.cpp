#include "files.hpp"

#include <fstream>

namespace aln::utils
{
std::vector<char> readFile(const std::string& filename)
{
    // We start to read at the end of the file so we can use the read position to determine the size of the file to allocate a buffer
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();

    std::vector<char> buffer(fileSize);

    // Go back to the start and read
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

std::string getFileExtension(const std::string& fileName)
{
    if (fileName.find_last_of(".") != std::string::npos)
        return fileName.substr(fileName.find_last_of(".") + 1);
    return "";
}
}
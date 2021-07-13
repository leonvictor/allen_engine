#pragma once

#include <string>
#include <vector>

namespace aln::utils
{
std::vector<char> readFile(const std::string& filename);
std::string getFileExtension(const std::string& fileName);
} // namespace aln::utils
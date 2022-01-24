#pragma once

#include <assert.h>
#include <cinttypes>
#include <string>
#include <vector>

namespace aln
{
class Graph
{
    uint32_t GetParameterIndex(std::string parameterName);
    void SetParameter(uint32_t parameterIdx, float value);
};
} // namespace aln
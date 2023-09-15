#pragma once

#include <nlohmann/json.hpp>

#include "../containers/vector.hpp"

namespace aln
{
using JSON = nlohmann::json;

// Vector
template <typename T>
void to_json(JSON& json, const Vector<T>& v)
{
    for (auto& e : v)
    {
        json.push_back(e);
    }
}

template <typename T>
static void from_json(const JSON& json, Vector<T>& v)
{
    v.reserve(json.size());
    for (auto& elementJson : json)
    {
        v.push_back(elementJson.get<T>());
    }
}

} // namespace aln
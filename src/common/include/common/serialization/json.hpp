#pragma once

#include "../containers/vector.hpp"

#include <nlohmann/json.hpp>

namespace aln
{

using JSON = nlohmann::json;

template <typename T>
static void ToJSON(JSON& json, const Vector<T>& v)
{
    for (auto& e : v)
    {
        json.push_back(e);
    }
}

template <typename T>
static void FromJSON(const JSON& json, Vector<T>& v)
{
    v.reserve(json.size());
    for (auto& elementJson : json)
    {
        v.push_back(elementJson.get<T>());
    }
}

} // namespace aln
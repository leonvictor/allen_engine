#pragma once

#include <unordered_map>
#include <vector>

namespace aln
{

/// @brief Contiguous container with fast lookup.
/// @todo Use a custom iterator keeping storing current index. Slight cost for performance improvements in case of find() -> erase() for example
template <typename ValueType, typename Hash = std::hash<ValueType>>
class HashVector
{
    using Iterator = std::vector<ValueType>::iterator;
    using KeyType = std::invoke_result_t<Hash, ValueType>;

  private:
    Hash hasher;
    std::vector<ValueType> m_vector;
    std::unordered_map<KeyType, size_t> m_lookupMap;

  public:
    size_t Size() const { return m_vector.size(); }
    bool Empty() const { return m_vector.empty(); }

    void Clear()
    {
        m_vector.clear();
        m_lookupMap.clear();
    }

    template <typename... Args>
    ValueType& TryEmplace(const KeyType& key, Args&&... args)
    {
        auto [it, emplaced] = m_lookupMap.try_emplace(key);
        if (emplaced)
        {
            it->second = m_vector.size();
            m_vector.emplace_back(std::forward<Args>(args)...);
        }
        return m_vector[it->second];
    }

    void Erase(const KeyType& key)
    {
        auto it = m_lookupMap.find(key);
        m_vector.erase(m_vector.begin() + it->second);
        m_lookupMap.erase(it);
    }

    void Erase(const ValueType& value) { Erase(hasher(value)); }

    Iterator begin() { return m_vector.begin(); }
    Iterator end() { return m_vector.end(); }

    ValueType& Get(const KeyType& key) { return m_vector[m_lookupMap[key]]; }
};

// TODO: Clients do no need this
namespace
{
template <typename T>
struct IDCompare
{
    auto operator()(const T& t) { return t.GetID(); }
};
}; // namespace

/// @brief Specialization of HashVector for ID'ed types. Expects the contained type to have a GetID() method returning a unique identifier
template <typename T>
using IDVector = HashVector<T, IDCompare<T>>;

} // namespace aln

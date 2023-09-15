#pragma once

#include <common/containers/vector.hpp>

#include <type_traits>
#include <unordered_map>

namespace aln
{

/// @brief Contiguous container with fast lookup.
/// @todo Use a custom iterator keeping storing current index. Slight cost for performance improvements in case of find() -> erase() for example
/// @todo Replace std containers
template <typename ValueType, typename Hash = std::hash<ValueType>>
class HashVector
{
    using Iterator = Vector<ValueType>::iterator;
    using ConstIterator = Vector<ValueType>::const_iterator;
    using KeyType = std::invoke_result_t<Hash, ValueType>;
    using SizeType = Vector<ValueType>::size_type;

  private:
    Hash hasher;
    Vector<ValueType> m_vector;
    std::unordered_map<KeyType, SizeType> m_lookupMap;

  public:
    size_t Size() const { return m_vector.size(); }
    bool Empty() const { return m_vector.empty(); }

    void Clear()
    {
        m_vector.clear();
        m_lookupMap.clear();
    }

    void PushBack(ValueType& value)
    {
        m_vector.push_back(value);
        m_lookupMap.emplace(hasher(value), m_vector.size() - 1);
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

    ValueType& EmplaceBack()
    {
        auto& element = m_vector.emplace_back();
        m_lookupMap[hasher(element)] = m_vector.size() - 1;
        return element;
    }

    /// @brief Erase an element. Element ordering is not preserved !
    Iterator Erase(Iterator it)
    {
        m_lookupMap.erase(hasher(*it));

        if (it != m_vector.end() - 1)
        {
            // Move the last element in the vector to the erased element idx
            *it = std::move(m_vector.back());
            // Update the lookup map
            m_lookupMap[hasher(*it)] = std::distance(m_vector.begin(), it);
            m_vector.pop_back();
            return it;
        }
        else
        {
            m_vector.pop_back();
            return m_vector.end();
        }
    }

    void Erase(const KeyType& key)
    {
        auto it = m_vector.begin() + m_lookupMap[key];
        Erase(it);
    }

    void Erase(const ValueType& value) { Erase(hasher(value)); }

    /// @brief Erase all elements matching the predicate. Element order is not preserved !
    template <typename Predicate>
    void EraseIf(Predicate predicate)
    {
        // TODO: Group "erase" calls in a single statement
        auto it = m_vector.begin();
        while (it != m_vector.end())
        {
            if (predicate(*it))
            {
                it = Erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    Iterator begin() { return m_vector.begin(); }
    Iterator end() { return m_vector.end(); }
    ConstIterator begin() const { return m_vector.begin(); }
    ConstIterator end() const { return m_vector.end(); }
    ConstIterator cbegin() const { return m_vector.cbegin(); }
    ConstIterator cend() const { return m_vector.cend(); }

    ValueType& Get(const KeyType& key) { return m_vector[m_lookupMap.at(key)]; }
    const ValueType& Get(const KeyType& key) const { return m_vector[m_lookupMap.at(key)]; }
    ValueType& operator[](SizeType index) { return m_vector[index]; }
    const ValueType& operator[](SizeType index) const { return m_vector[index]; }
};

// TODO: Clients do no need this
namespace
{
template <typename T>
struct IDCompare
{
    auto operator()(const T& t) { return t.GetID(); }
    auto operator()(const T* ptr) { return ptr->GetID(); }
};
}; // namespace

/// @brief Specialization of HashVector for ID'ed types. Expects the contained type to have a GetID() method returning a unique identifier
template <typename T>
using IDVector = HashVector<T, IDCompare<std::remove_pointer_t<T>>>;

} // namespace aln

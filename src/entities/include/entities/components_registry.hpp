#pragma once

#include <map>
#include <vector>

#include <utils/uuid.hpp>

#include "component.hpp"

namespace aln::entities
{

// fwd
template <typename T>
class ComponentsRegistry;

/// @brief A registry holds a register mapping entities IDs to their recorded components.
template <typename T>
class ComponentsRegistry
{
    class iterator
    {
        friend class ComponentsRegistry;
        using value_type = T;

      private:
        ComponentsRegistry<T>* m_pRegistry = nullptr;

        std::map<aln::utils::UUID, std::vector<T*>>::iterator m_mapIterator;
        std::vector<T*>::iterator m_vecIterator;

        iterator(ComponentsRegistry<T>* pRegistry) : m_pRegistry(pRegistry),
                                                     m_mapIterator(m_pRegistry->m_registry.begin()),
                                                     m_vecIterator(m_pRegistry->m_registry.empty() ? std::vector<T*>::iterator() : m_pRegistry->m_registry.begin()->second.begin()) {}

        void ToEnd()
        {
            m_mapIterator = m_pRegistry->m_registry.end();
            if (m_pRegistry->m_registry.empty())
            {
                m_vecIterator = std::vector<T*>::iterator();
            }
            else
            {
                m_mapIterator--;
                m_vecIterator = m_mapIterator->second.end();
                m_mapIterator++;
            }
        }

      public:
        iterator& operator++()
        {
            m_vecIterator++;

            if (m_vecIterator == m_mapIterator->second.end())
            {
                m_mapIterator++;
                if (m_mapIterator != m_pRegistry->m_registry.end())
                {
                    m_vecIterator = m_mapIterator->second.begin();
                }
            }
            return *this;
        }

        iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        T* operator*() { return *m_vecIterator; }
        T** operator->() { return *m_vecIterator; }

        friend bool operator==(const iterator& a, const iterator& b)
        {
            return a.m_mapIterator == b.m_mapIterator && a.m_vecIterator == b.m_vecIterator;
        }

        friend bool operator!=(const iterator& a, const iterator& b)
        {
            return !(a == b);
        }
    };

  private:
    size_t m_registeredComponentsCount = 0;
    std::map<aln::utils::UUID, std::vector<T*>> m_registry;

  public:
    void AddRecordEntry(const aln::utils::UUID id, T* pComponent)
    {
        // auto record = m_registry.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple());
        auto record = m_registry.try_emplace(id);
        record.first->second.push_back(pComponent);
        m_registeredComponentsCount++;
    }

    void RemoveRecordEntry(const aln::utils::UUID& id, T* pComponent)
    {
        auto recordIt = m_registry.find(id);
        if (recordIt != m_registry.end())
        {
            auto& entries = recordIt->second;
            auto entryIt = std::find_if(entries.begin(), entries.end(),
                [&](T* pRegisteredComponent)
                {
                    return pRegisteredComponent == pComponent;
                });

            if (entryIt != entries.end())
            {
                entries.erase(entryIt);
                m_registeredComponentsCount--;
            }

            if (recordIt->second.empty())
            {
                m_registry.erase(recordIt);
            }
        }
    }

    size_t GetRegisterComponentsCount() { return m_registeredComponentsCount; }

    iterator begin() { return iterator(this); }

    iterator end()
    {
        auto it = begin();
        it.ToEnd();
        return it;
    }
};

} // namespace aln::entities
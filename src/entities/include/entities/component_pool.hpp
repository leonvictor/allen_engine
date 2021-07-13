#pragma once

#include "component.hpp"
#include <vector>

namespace aln::entities
{
template <typename T>
class ComponentPool
{
    /// @brief Holds all components of type T in a contiguous array
    static std::vector<T> m_components;
};

class ComponentFactory
{
    template <typename T>
    T* Create()
    {
        ComponentPool<T>::m_components
    }
}
}
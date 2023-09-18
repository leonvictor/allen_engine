#pragma once

#include "service.hpp"

#include <common/containers/hash_map.hpp>

#include <typeindex>


namespace aln
{
class ServiceProvider
{
  private:
    HashMap<std::type_index, IService*, std::hash<std::type_index>> m_services;

  public:
    ServiceProvider() {}

    template <typename T>
    void RegisterService(T* pService)
    {
        static_assert(std::is_base_of_v<IService, T>);

        auto typeID = std::type_index(typeid(T));
        auto [it, emplaced] = m_services.try_emplace(typeID, pService);
        assert(emplaced);
    }

    template <typename T>
    T* GetService()
    {
        auto typeID = std::type_index(typeid(T));
        return (T*) m_services[typeID];
    }
};
} // namespace aln
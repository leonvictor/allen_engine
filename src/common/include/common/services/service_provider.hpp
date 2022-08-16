#pragma once

#include <map>
#include <typeindex>

#include "service.hpp"

namespace aln
{
class ServiceProvider
{
  private:
    std::map<std::type_index, IService*> m_services;

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
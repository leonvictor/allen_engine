#pragma once

#include "service.hpp"

#include <common/containers/hash_map.hpp>
#include <common/memory.hpp>

#include <typeindex>

namespace aln
{

class RenderEngine;

/// @brief Holds all singleton services. Used for dependency injection throughout the engine
class ServiceProvider
{
    friend class Engine;

  private:
    HashMap<std::type_index, IService*, std::hash<std::type_index>> m_services;

    /// @note Provider also holds references to shared resources to facilitate initialization
    RenderEngine* m_pRenderEngine = nullptr;

  private:
    void Initialize(RenderEngine* pRenderEngine) { m_pRenderEngine = pRenderEngine; }

    void Shutdown()
    {
        assert(m_services.empty());
        m_pRenderEngine = nullptr;
    }

  public:
    /// @brief Create a service of type T
    /// @note I'd like to name this method "CreateService" but the names gets mangled by a macro in winsvc.h
    /// @todo Hide from clients
    template <typename T>
    T* AddService()
    {
        static_assert(std::is_base_of_v<IService, T>);

        auto typeID = std::type_index(typeid(T));
        auto [it, emplaced] = m_services.try_emplace(typeID);
        assert(emplaced);

        auto pService = aln::New<T>();
        pService->Initialize(this);
        assert(pService->IsRegisteredWithProvider());

        it->second = pService;

        return pService;
    }

    template <typename T>
    void RemoveService()
    {
        static_assert(std::is_base_of_v<IService, T>);
        
        /// @fixme Double lookups
        auto typeIndex = std::type_index(typeid(T));
        auto pService = m_services[typeIndex];
        pService->Shutdown();
        aln::Delete(pService);
        m_services.erase(typeIndex);
    }

    template <typename T>
    T* GetService() const
    {
        auto typeID = std::type_index(typeid(T));
        return (T*) m_services.at(typeID);
    }

    RenderEngine* GetRenderEngine() { return m_pRenderEngine; }
};
} // namespace aln
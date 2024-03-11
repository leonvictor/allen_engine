#pragma once

#include "../world_entity.hpp"

#include <common/containers/vector.hpp>
#include <common/services/service.hpp>

namespace aln
{
/// @brief Keeps track of active worlds. Only one game world is active a any time, the others might be used by editor previews for example.
class WorldsService : public IService
{
    friend class ServiceProvider;

  private:
    Vector<WorldEntity*> m_worlds;
    uint32_t m_gameWorldIdx = InvalidIndex;

  private:
    void Shutdown() override
    {
        for (auto pWorld : m_worlds)
        {
            pWorld->Shutdown();
            aln::Delete(pWorld);
        }
        m_worlds.clear();
        m_gameWorldIdx = InvalidIndex;
    }

  public:
    const Vector<WorldEntity*>& GetWorlds() const { return m_worlds; }

    WorldEntity* GetGameWorld() { return m_worlds[m_gameWorldIdx]; }
    const WorldEntity* GetGameWorld() const { return m_worlds[m_gameWorldIdx]; }

    /// @brief Create a new world entity. Only one game world can exist at once
    /// @todo Prevent creation of world entities in any other way
    WorldEntity* CreateWorld(bool isGameWorld)
    {
        // Only one game world allowed
        assert(!isGameWorld || m_gameWorldIdx == InvalidIndex);

        auto pWorld = aln::New<WorldEntity>();
        pWorld->Initialize(*GetProvider());
        m_worlds.push_back(pWorld);

        if (isGameWorld)
        {
            m_gameWorldIdx = m_worlds.size() - 1;
        }

        return pWorld;
    }

    void DestroyWorld(WorldEntity* pWorld)
    {
        const auto it = m_worlds.erase_first(pWorld);
        pWorld->Shutdown();
        aln::Delete(pWorld);
    }


    void UpdateLoading()
    {
        for (auto& pWorld : m_worlds)
        {
            pWorld->UpdateLoading();
        }
    }

    void Update(const UpdateContext& ctx)
    {
        for (auto& pWorld : m_worlds)
        {
            pWorld->Update(ctx);
        }
    }
};
} // namespace aln
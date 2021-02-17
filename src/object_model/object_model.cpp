#include "entity_system.cpp" // For update stages (move if necessary)

class Entity;
class Component;

namespace ObjectModel
{
class LoadingContext
{
  public:
    // TODO !!!!
    void m_unregisterWithGlobalSystems(Entity* pEntity, Component* pComponent) const {}
    void m_registerWithGlobalSystems(Entity* pEntity, Component* pComponent) const {}
    void m_unregisterEntityUpdate(Entity* pEntity) const {}
    void m_registerEntityUpdate(Entity* pEntity) const {}
};

class UpdateContext
{
  private:
    UpdateStage m_updateStage = UpdateStage::FrameStart; // TODO

  public:
    UpdateStage GetUpdateStage() const
    {
        return m_updateStage;
    }
};
} // namespace ObjectModel
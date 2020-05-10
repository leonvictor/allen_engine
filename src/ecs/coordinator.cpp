#include "common.cpp"
#include "component_manager.hpp"
#include "entity_manager.cpp"
#include "system_manager.hpp"

class Coordinator
{
public:
	void init()
	{
		// Create pointers to each manager
		mComponentManager = std::make_unique<ecs::ComponentManager>();
		mEntityManager = std::make_unique<ecs::EntityManager>();
		mSystemManager = std::make_unique<ecs::SystemManager>();
	}


	// Entity methods
	ecs::Entity createEntity()
	{
		return mEntityManager->createEntity();
	}

	void destroyEntity(ecs::Entity entity)
	{
		mEntityManager->destroyEntity(entity);

		mComponentManager->entityDestroyed(entity);

		mSystemManager->entityDestroyed(entity);
	}


	// Component methods
	template<typename T>
	void registerComponent()
	{
		mComponentManager->registerComponent<T>();
	}

	template<typename T>
	void addComponent(ecs::Entity entity, T component)
	{
		mComponentManager->addComponent<T>(entity, component);

		auto signature = mEntityManager->getSignature(entity);
		signature.set(mComponentManager->getComponentType<T>(), true);
		mEntityManager->setSignature(entity, signature);

		mSystemManager->entitySignatureChanged(entity, signature);
	}

	template<typename T>
	void removeComponent(ecs::Entity entity)
	{
		mComponentManager->removeComponent<T>(entity);

		auto signature = mEntityManager->getSignature(entity);
		signature.set(mComponentManager->getComponentType<T>(), false);
		mEntityManager->setSignature(entity, signature);

		mSystemManager->entitySignatureChanged(entity, signature);
	}

	template<typename T>
	T& getComponent(ecs::Entity entity)
	{
		return mComponentManager->getComponent<T>(entity);
	}

	template<typename T>
	ecs::ComponentType getComponentType()
	{
		return mComponentManager->getComponentType<T>();
	}


	// System methods
	template<typename T>
	std::shared_ptr<T> registerSystem()
	{
		return mSystemManager->registerSystem<T>();
	}

	template<typename T>
	void setSystemSignature(ecs::Signature signature)
	{
		mSystemManager->setSignature<T>(signature);
	}

private:
	std::unique_ptr<ecs::ComponentManager> mComponentManager;
	std::unique_ptr<ecs::EntityManager> mEntityManager;
	std::unique_ptr<ecs::SystemManager> mSystemManager;
};

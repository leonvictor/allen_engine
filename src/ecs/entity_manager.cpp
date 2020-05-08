// Entity Component System inspired by https://austinmorlan.com/posts/entity_component_system/
#include <array>
#include <queue>
#include <bitset>
#include <assert.h>
#include "common.cpp"

namespace ecs {

    class EntityManager {
        private:
        std::queue<Entity> mAvailableEntities;
        std::array<Signature, MAX_ENTITIES> mSignatures;
        uint32_t mLivingEntityCount = 0;
        
        public:
        EntityManager() {
            for (Entity entity = 0; entity < MAX_ENTITIES; entity++) {
                mAvailableEntities.push(entity);
            }
        }

        Entity createEntity() {
            assert(mLivingEntityCount < MAX_ENTITIES && "Maximum entities reached");

            Entity id = mAvailableEntities.front();
            mAvailableEntities.pop();
            mLivingEntityCount++;

            return id;
        }

        void DestroyEntity(Entity entity) {
            assert(entity < MAX_ENTITIES && "Entity out of range");

            mSignatures[entity].reset();
            mAvailableEntities.push(entity);
            mLivingEntityCount--;
        }

        void setSignature(Entity entity, Signature signature) {
            assert(entity < MAX_ENTITIES && "Entity ouf of range");
            mSignatures[entity] = signature;
        }

        Signature getSignature(Entity entity) {
            assert(entity < MAX_ENTITIES && "Entity out of range");
            return mSignatures[entity];
        }
    };
}
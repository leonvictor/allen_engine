// #include "common.cpp"
// #include "coordinator.cpp"
// #include "components.hpp"
// #include "../camera.cpp"
// #include "../core/context.hpp"
// #include "glm/glm.hpp"

// extern Coordinator gCoordinator; // TODO: Global variable are poopy.
// // We could set the coordinator while registering the system

// class RenderSystem : public ecs::System {
//     public:

//     void createDescriptor(ecs::Entity entity) {
//         auto& renderable = gCoordinator.getComponent<ecs::components::Renderable>(entity);

//     }

//     void update(const Camera& camera, const float aspectRatio) {
//         // aspectRatio = swapchain.extent.width / (float) swapchain.extent.height
//         for (auto const& entity : mEntities) {
//             auto& transform = gCoordinator.getComponent<ecs::components::Transform>(entity);
//             auto& renderable = gCoordinator.getComponent<ecs::components::Renderable>(entity);

//             // TODO: Where do we put the UBO ? in a "renderable" component ?

//             glm::mat4 modelMatrix = glm::mat4(1.0f);
//             modelMatrix = glm::translate(modelMatrix, transform.position);

//             core::UniformBufferObject ubo;
//             ubo.model = modelMatrix;
//             ubo.view = camera.getViewMatrix(); // eye/camera position, center position, up axis
//             ubo.projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
//             ubo.projection[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
//             ubo.cameraPos = camera.position;

//             renderable.uniform.map(0, sizeof(ubo));
//             renderable.uniform.copy(&ubo, sizeof(ubo));
//             renderable.uniform.unmap();
//         }
//     }
// };

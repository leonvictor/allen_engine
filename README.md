# vulkan_sandbox

My experiments using Vulkan. I plan to make a simple renderer to begin with, then I'll switch my focus to tooling (especially Animation)
The first raw edition of the program was developped following [this tutorial](https://vulkan-tutorial.com/).

# TODO
 - Use dynamic states for viewport and scissors to make window resizing more efficient
 - Improve shaders compilation (for now we have to do it manually beforehand)
 - "The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects by using the offset parameters that we've seen in many functions. You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative. However, for this tutorial it's okay to use a separate allocation for every resource, because we won't come close to hitting any of these limits for now.""
 - cont. [Store multiple buffers into a single VkBuffer and use offsets (aliasing)](https://developer.nvidia.com/vulkan-memory-management)
 - Font Rendering
 - Dear Imgui panels
 - Fix camera gimbal lock
 - Asset loading with assimp
 - Pick and stick to a clang format (w/ assorted config file)
 - Improve lighting in many many ways : 
    - Support lighting maps
    - ~~Different light types~~ (Still missing spot lights)
    - ~~Multiple lights in scene~~
 - PBR
 - Add specialization constants to specify light types to the pipeline (and the shaders) at creation time
 - Unify naming to using a single convention (Google ?)
 - Create a custom ECS framework

Ideas of todos proposed by the tutorial :

 - Push constants
 - Instanced rendering
 - Dynamic uniforms [(see this link)](https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer)
 - Separate images and sampler descriptors
 - ~~Pipeline cache~~
 - Multi-threaded command buffer generation
 - Multiple subpasses
 - Compute shaders
 - Post processing effects
 - Shadow mapping
 - Blinn- ~~Phong lighting~~

# Dependencies
 - Vulkan SDK
 - glm
 - glfw
 - tinyobjloader
 - stb
 - entt

# Assets
Skyboxes from : https://github.com/PhannGor/Cloudy-Crown
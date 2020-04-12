# vulkan_sandbox

My experiments using Vulkan. I plan to make a simple renderer to begin with, then I'll switch my focus to tooling (especially Animation)
The first raw edition of the program was developped following [this tutorial](https://vulkan-tutorial.com/).

# TODO
 - Configure CMake
 - Separate main.cpp in focused files
 - Use dynamic states for viewport and scissors to make window resizing more efficient
 - Improve shaders compilation (for now we have to do it manually beforehand)
 - "The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects by using the offset parameters that we've seen in many functions. You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative. However, for this tutorial it's okay to use a separate allocation for every resource, because we won't come close to hitting any of these limits for now.""
 - cont. [Store multiple buffers into a single VkBuffer and use offsets (aliasing)](https://developer.nvidia.com/vulkan-memory-management)

Ideas of todos proposed by the tutorial :

 - Push constants
 - Instanced rendering
 - Dynamic uniforms
 - Separate images and sampler descriptors
 - Pipeline cache
 - Multi-threaded command buffer generation
 - Multiple subpasses
 - Compute shaders
 - Post processing effects
 - Shadow mapping
 - Blinn-Phong lighting


# My notes
For now explicit memory alloc, but it is recommended to use RAII (Resource Acquisition is Initialization) later on.
VkCreate, VkAllocate, VkDestroy and vkFree have a parameter pAllocator used to specify callbacks for a custom memory allocator. Not used in the tutorial
SPIR-V is a bytecode format for shader code, as opposed to human readable syntaxes like GLSL and HLSL. Khronos provides a compiler for GLSL -> SPIR-V. It is possible to ship it w/ the app to compule shaders at runtime but we don't do that in this tutorial.
We can use this compiler via glslangValidator.exe but we'll use glslc.exe instead. Provided by google, it has the advantage of using the same parameter format as GCC.

We use a manual compile script  (shaders/compile.bat) in the tutorial, but it is possible to compile them from the code. See libshaderc. 

Fences : Designed to synchronize the application itself with rendering (can be accessed from the prog)
Semaphores : Synchronize operation within or accross command queues

# Dependencies
 - Vulkan SDK
 - glm
 - glfw
 - tinyobjloader
 - stb
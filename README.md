# vulkan_sandbox

My experiments using Vulkan. I plan to make a simple renderer to begin with, then I'll switch my focus to tooling (especially Animation)
For now i'm following a tutorial.

# TODO
 - Configure CMake

# My notes
For now explicit memory alloc, but it is recommended to use RAII (Resource Acquisition is Initialization) later on
VkCreate, VkAllocate, VkDestroy and vkFree have a parameter pAllocator used to specify callbacks for a custom memory allocator. Not used in the tutorial
SPIR-V is a bytecode format for shader code, as opposed to human readable syntaxes like GLSL and HLSL. Khronos provides a compiler for GLSL -> SPIR-V. It is possible to ship it w/ the app to compule shaders at runtime but we don't do that in this tutorial.
We can use this compiler viea glslangValidator.exe but we'll use glslc.exe instead. Provided by google, it has the advantage of using the same parameter format as GCC.

We use a manual compile script  (shaders/compile.bat) in the tutorial, but it is possible to compile them from the code. See libshaderc. 

Fences : Designed to synchronize the application itself with rendering (can be accessed from the prog)
Semaphores : Synchronize operation within or accross command queues

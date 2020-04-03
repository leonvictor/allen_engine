# vulkan_sandbox

My experiments using Vulkan. I plan to make a simple renderer to begin with, then I'll switch my focus to tooling (especially Animation)
For now i'm following a tutorial.

# TODO
 - Configure CMake

# My notes
For now explicit memory alloc, but it is recommended to use RAII (Resource Acquisition is Initialization) later on
VkCreate, VkAllocate, VkDestroy and vkFree have a parameter pAllocator used to specify callbacks for a custom memory allocator. Not used in the tutorial


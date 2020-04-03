#undef GLFW_DLL
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
// #include <GLFW/glfw3.h>
// #include <cstdlib>
// #include <iostream>
// using namespace std;

// int main() {
//   if (!glfwInit()) {
//     cerr << "Can't initialize GLFW" << endl;
//     exit (EXIT_FAILURE);
//   }

//   GLFWwindow *win;
//   win = glfwCreateWindow (450, 300, "Hello", NULL, NULL);
//   glfwMakeContextCurrent(win);
//   while (!glfwWindowShouldClose(win)) {
//     glfwWaitEvents();
//   }
//   glfwDestroyWindow(win);
//   glfwTerminate();
//   return 0;
// }

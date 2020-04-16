#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
    public:
        glm::vec3 position;
        glm::vec3 forward;

        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f); // TODO: Grab this from a program-wide const

        float fov;
        float yaw, pitch, roll;

        float mouseSensitivity;
        float scrollSensitivity;

        Camera(glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = 0.0f, float pitch = 0.0f, float roll = 0.0f) : 
            position(position), up(up), yaw(yaw), pitch(pitch), roll(roll) {
                updateCameraVectors();
            }

    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        yaw += xoffset;
        pitch += yoffset;

        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + forward, up);
    }

    private:
        void updateCameraVectors() {
            forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            forward.y = sin(glm::radians(pitch));
            forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            forward = glm::normalize(forward);
    
            right = glm::normalize(glm::cross(forward, world_up));
            up    = glm::normalize(glm::cross(right, forward)); 
        }
};
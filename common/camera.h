#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

class Camera {
public:
    GLFWwindow* window;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    // Fixed establishing shot
    glm::vec3 position;
    float horizontalAngle;
    float verticalAngle;
    float FoV;

    Camera(GLFWwindow* window);
    void update();
    void resetToEstablishingShot();
};

#endif

#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

using namespace glm;

Camera::Camera(GLFWwindow* window) : window(window) {
    resetToEstablishingShot();
}

void Camera::resetToEstablishingShot() {
    // Keep the river as a leading line and the crater against open sky.
    position = vec3(235.0f, 365.0f, 1040.0f);
    const vec3 target(-35.0f, 125.0f, -285.0f);
    const vec3 direction = normalize(target - position);
    horizontalAngle = atan2(direction.x, direction.z);
    verticalAngle = asin(direction.y);
    FoV = 43.0f;
}

void Camera::update() {
    // The view stays at the establishing shot; only the framebuffer aspect changes.
    vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );

    vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );
    vec3 up = cross(right, direction);

    // Projection must follow framebuffer pixels rather than a fixed 4:3 ratio.
    // This also handles Retina/HiDPI windows where logical and drawable sizes differ.
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    const float aspectRatio = framebufferHeight > 0
        ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
        : 4.0f / 3.0f;

    projectionMatrix = perspective(radians(FoV), aspectRatio, 0.1f, 10000.0f);
    viewMatrix = lookAt(
        position,
        position + direction,
        up
    );
}

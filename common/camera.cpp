#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

using namespace glm;

Camera::Camera(GLFWwindow* window) : window(window) {
    speed = 280.0f;
    mouseSpeed = 0.001f;
    fovSpeed = 35.0f;
    mouseLookEnabled = true;
    resetToEstablishingShot();
}

void Camera::resetToEstablishingShot() {
    // A slightly off-centre view makes the river a leading line while keeping the
    // crater against open sky. These values are deliberately explicit so the shot
    // is easy to reproduce and explain during the project presentation.
    position = vec3(235.0f, 365.0f, 1040.0f);
    const vec3 target(-35.0f, 125.0f, -285.0f);
    const vec3 direction = normalize(target - position);
    horizontalAngle = atan2(direction.x, direction.z);
    verticalAngle = asin(direction.y);
    FoV = 43.0f;
}

void Camera::setMouseLookEnabled(bool enabled) {
    mouseLookEnabled = enabled;
    glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (enabled) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetCursorPos(window, width / 2, height / 2);
    }
}

void Camera::update() {
    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    if (mouseLookEnabled) {
        // Reset mouse position for next frame and convert the offset to view angles.
        glfwSetCursorPos(window, width / 2, height / 2);
        horizontalAngle += mouseSpeed * float(width / 2 - xPos);
        verticalAngle += mouseSpeed * float(height / 2 - yPos);
        verticalAngle = glm::clamp(verticalAngle, -1.45f, 1.45f);
    }

    // Derive the camera basis from spherical look angles.
    vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );

    // Right vector
    vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    vec3 up = cross(right, direction);

    // Move in the current view plane.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= right * deltaTime * speed;
    }

    // Frame-rate-independent zoom with a range that keeps the perspective valid.
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        FoV -= fovSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        FoV += fovSpeed * deltaTime;
    }
    FoV = glm::clamp(FoV, 20.0f, 90.0f);

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
    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

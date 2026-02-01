#include "SceneDirector.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glfw3.h>
#include <common/camera.h>

SceneDirector::SceneDirector(Camera* camera, Light* moonlight, Volcano* volcano, Skybox* skybox)
    : m_camera(camera),
      m_moonlight(moonlight),
      m_volcano(volcano),
      m_skybox(skybox),
      m_startTime(-1.0),
      m_lastTime(0.0),
      m_paused(false),
      m_shakeStartSeconds(3.0),
      m_lavaStartSeconds(7.0),
      m_lavaTimeSeconds(0.0f),
      m_lastShakeOffset(0.0f),
      m_hasShakeOffset(false) {}

void SceneDirector::reset(double timeSeconds) {
    m_startTime = timeSeconds;
    m_lastTime = timeSeconds;
}

void SceneDirector::setPaused(bool paused) {
    m_paused = paused;
}

bool SceneDirector::isPaused() const {
    return m_paused;
}

float SceneDirector::getLavaTimeSeconds() const {
    return m_lavaTimeSeconds;
}

void SceneDirector::update(double timeSeconds, float deltaSeconds) {
    if (m_startTime < 0.0) {
        reset(timeSeconds);
    }

    if (m_paused) {
        m_lastTime = timeSeconds;
        return;
    }

    if (m_camera && m_hasShakeOffset) {
        m_camera->position -= m_lastShakeOffset;
        m_lastShakeOffset = glm::vec3(0.0f);
        m_hasShakeOffset = false;
    }

    if (m_camera) {
        m_camera->update();
    }

    double elapsed = timeSeconds - m_startTime;
    m_lastTime = timeSeconds;

    m_lavaTimeSeconds = -1000.0f;
    if (elapsed >= m_lavaStartSeconds) {
        m_lavaTimeSeconds = static_cast<float>(elapsed - m_lavaStartSeconds);
    }

    if (m_camera && elapsed >= m_shakeStartSeconds && elapsed < m_lavaStartSeconds) {
        double shakeElapsed = elapsed - m_shakeStartSeconds;
        float t = static_cast<float>(shakeElapsed);
        float rampUp = glm::clamp(static_cast<float>(shakeElapsed / 0.75), 0.0f, 1.0f);
        float rampDown = glm::clamp(static_cast<float>((m_lavaStartSeconds - elapsed) / 0.75), 0.0f, 1.0f);
        float strength = 2.0f * rampUp * rampDown;

        glm::vec3 offset(
            std::sin(t * 11.0f),
            std::sin(t * 13.0f),
            std::cos(t * 9.0f)
        );
        offset *= strength;

        m_camera->position += offset;
        m_lastShakeOffset = offset;
        m_hasShakeOffset = true;

        glm::vec3 direction(
            std::cos(m_camera->verticalAngle) * std::sin(m_camera->horizontalAngle),
            std::sin(m_camera->verticalAngle),
            std::cos(m_camera->verticalAngle) * std::cos(m_camera->horizontalAngle)
        );
        glm::vec3 right(
            std::sin(m_camera->horizontalAngle - 3.14f / 2.0f),
            0.0f,
            std::cos(m_camera->horizontalAngle - 3.14f / 2.0f)
        );
        glm::vec3 up = glm::cross(right, direction);

        m_camera->viewMatrix = glm::lookAt(
            m_camera->position,
            m_camera->position + direction,
            up
        );
    }

    (void)m_moonlight;
    (void)m_volcano;
    (void)m_skybox;
    (void)deltaSeconds;
}

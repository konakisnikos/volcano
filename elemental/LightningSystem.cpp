#include "LightningSystem.h"
#include <common/shader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

LightningSystem::LightningSystem()
    : m_vao(0), m_vbo(0), m_shader(0), m_vLoc(-1), m_pLoc(-1),
      m_colorLoc(-1), m_active(false), m_impactPending(false),
      m_impactCreated(false), m_strikeStartTime(0.0f), m_visualAge(0.0f),
      m_flashStrength(0.0f),
      m_cloudPosition(0.0f), m_strikePosition(0.0f) {
    m_shader = loadShaders(ELEMENTAL_SHADER_DIR "/Lightning.vertexshader",
                           ELEMENTAL_SHADER_DIR "/Lightning.fragmentshader");
    m_vLoc = glGetUniformLocation(m_shader, "V");
    m_pLoc = glGetUniformLocation(m_shader, "P");
    m_colorLoc = glGetUniformLocation(m_shader, "uColor");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

LightningSystem::~LightningSystem() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_shader) glDeleteProgram(m_shader);
}

void LightningSystem::trigger(float simulationTime, const glm::vec3& cloudPosition,
                              const glm::vec3& strikePosition) {
    m_active = true;
    m_impactPending = false;
    m_impactCreated = false;
    m_strikeStartTime = simulationTime;
    m_visualAge = 0.0f;
    m_cloudPosition = cloudPosition;
    m_strikePosition = strikePosition;
    buildBoltGeometry();
}

void LightningSystem::reset() {
    m_active = false;
    m_impactPending = false;
    m_impactCreated = false;
    m_visualAge = 0.0f;
    m_flashStrength = 0.0f;
    m_mainPoints.clear();
    m_branches.clear();
    m_vertices.clear();
}

void LightningSystem::buildBoltGeometry() {
    m_mainPoints.clear();
    m_branches.clear();

    constexpr int SEGMENTS = 18;
    const glm::vec3 boltVector = m_strikePosition - m_cloudPosition;
    const glm::vec3 forward = glm::normalize(boltVector);
    const glm::vec3 reference = std::abs(glm::dot(forward, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.92f
        ? glm::vec3(1.0f, 0.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 sideA = glm::normalize(glm::cross(forward, reference));
    const glm::vec3 sideB = glm::normalize(glm::cross(forward, sideA));
    const float seed = m_strikePosition.x * 0.071f
                     + m_strikePosition.z * 0.043f
                     + m_strikeStartTime * 0.19f;

    m_mainPoints.reserve(SEGMENTS + 1);
    for (int i = 0; i <= SEGMENTS; ++i) {
        const float t = static_cast<float>(i) / SEGMENTS;
        glm::vec3 point = glm::mix(m_cloudPosition, m_strikePosition, t);
        if (i != 0 && i != SEGMENTS) {
            const float envelope = std::sin(t * 3.14159265f);
            const float jagA = std::sin(seed + i * 12.73f)
                             + 0.45f * std::sin(seed * 1.7f + i * 5.19f);
            const float jagB = std::cos(seed * 0.83f + i * 9.31f);
            point += sideA * jagA * 13.5f * envelope;
            point += sideB * jagB * 7.5f * envelope;
        }
        m_mainPoints.push_back(point);
    }

    // Three short secondary forks add the branching silhouette associated with
    // lightning without increasing the geometry or draw-call count noticeably.
    const int branchStarts[] = { 5, 9, 13, 15 };
    for (int branchIndex = 0; branchIndex < 4; ++branchIndex) {
        const int startIndex = branchStarts[branchIndex];
        const float sign = branchIndex % 2 == 0 ? -1.0f : 1.0f;
        const float length = 34.0f + branchIndex * 7.0f;
        glm::vec3 branchDirection = glm::normalize(
            sideA * sign + sideB * (0.25f + branchIndex * 0.08f)
            + forward * 0.28f);

        BranchPath branch;
        branch.appearAfterSegment = startIndex;
        branch.points.push_back(m_mainPoints[startIndex]);
        branch.points.push_back(m_mainPoints[startIndex]
                              + branchDirection * length * 0.48f
                              + sideB * std::sin(seed + branchIndex * 2.3f) * 7.0f);
        branch.points.push_back(m_mainPoints[startIndex]
                              + branchDirection * length
                              - sideA * sign * 5.0f);
        m_branches.push_back(branch);
    }
}

bool LightningSystem::consumeImpact() {
    if (!m_impactPending) return false;
    m_impactPending = false;
    return true;
}

void LightningSystem::update(float simulationTime, float simulationDelta) {
    m_vertices.clear();
    m_flashStrength = 0.0f;
    if (!m_active) return;

    // Simulation speed controls when strikes happen, but a high multiplier
    // must not compress a bolt into a single invisible frame. Capping only its
    // visual time step preserves a readable strike while pause (delta = 0)
    // still freezes it correctly.
    m_visualAge += glm::min(simulationDelta, 0.05f);
    const float age = m_visualAge;
    if (age < 0.0f || age > 0.68f) {
        m_active = false;
        return;
    }

    // The bolt grows from cloud to ground over its first 0.08 seconds. This
    // makes its travel readable even though the whole strike remains very fast.
    const int segments = static_cast<int>(m_mainPoints.size()) - 1;
    const int visibleSegments = glm::clamp(
        static_cast<int>(std::ceil((age / 0.08f) * segments)), 1, segments);

    if (visibleSegments == segments && !m_impactCreated) {
        m_impactCreated = true;
        m_impactPending = true;
    }

    const float flashEnvelope = glm::smoothstep(0.02f, 0.12f, age)
                              * (1.0f - glm::smoothstep(0.38f, 0.68f, age));
    const float flicker = 0.74f + 0.26f * std::abs(std::sin(age * 82.0f));
    m_flashStrength = flashEnvelope * flicker * 0.68f;

    for (int i = 0; i < visibleSegments; ++i) {
        m_vertices.push_back(m_mainPoints[i]);
        m_vertices.push_back(m_mainPoints[i + 1]);
    }
    for (const BranchPath& branch : m_branches) {
        if (visibleSegments < branch.appearAfterSegment) continue;
        for (std::size_t i = 0; i + 1 < branch.points.size(); ++i) {
            m_vertices.push_back(branch.points[i]);
            m_vertices.push_back(branch.points[i + 1]);
        }
    }
    (void)simulationTime;
}

void LightningSystem::draw(const glm::mat4& view, const glm::mat4& projection) {
    if (m_vertices.empty()) return;
    glUseProgram(m_shader);
    glUniformMatrix4fv(m_vLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(m_pLoc, 1, GL_FALSE, &projection[0][0]);
    const glm::vec3 cameraPosition = glm::vec3(glm::inverse(view)[3]);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Build camera-facing ribbons instead of relying on glLineWidth, which is
    // commonly clamped to one pixel on core-profile/macOS OpenGL drivers.
    auto drawRibbon = [&](float halfWidth, const glm::vec4& color) {
        std::vector<glm::vec3> ribbonVertices;
        ribbonVertices.reserve(m_vertices.size() * 3);
        for (std::size_t i = 0; i + 1 < m_vertices.size(); i += 2) {
            const glm::vec3 a = m_vertices[i];
            const glm::vec3 b = m_vertices[i + 1];
            const glm::vec3 direction = glm::normalize(b - a);
            glm::vec3 side = glm::cross(direction,
                                        cameraPosition - (a + b) * 0.5f);
            if (glm::dot(side, side) < 0.0001f) {
                side = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            side = glm::normalize(side) * halfWidth;

            ribbonVertices.push_back(a - side);
            ribbonVertices.push_back(b + side);
            ribbonVertices.push_back(a + side);
            ribbonVertices.push_back(a - side);
            ribbonVertices.push_back(b - side);
            ribbonVertices.push_back(b + side);
        }

        glUniform4fv(m_colorLoc, 1, &color[0]);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     ribbonVertices.size() * sizeof(glm::vec3),
                     &ribbonVertices[0], GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(ribbonVertices.size()));
    };

    // Wide translucent pass approximates bloom without a post-processing stage.
    drawRibbon(2.25f, glm::vec4(0.18f, 0.38f, 1.0f, 0.22f));
    // Thin near-white core keeps the jagged path crisp over clouds and terrain.
    drawRibbon(0.42f, glm::vec4(0.82f, 0.93f, 1.0f, 0.98f));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
}

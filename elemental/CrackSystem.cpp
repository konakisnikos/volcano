#include "CrackSystem.h"

#include <algorithm>
#include <cmath>
#include <random>

#include <elemental/elements/Volcano.h>

// Max cracks supported by the shader uniform arrays.
static constexpr int MAX_CRACKS = 8;

static std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

float CrackSystem::randRange(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng());
}

CrackSystem::CrackSystem() = default;

void CrackSystem::reset() {
    m_cracks.clear();
    m_created = false;
}

void CrackSystem::addCrack(const VolcanoStats& stats,
                           const CrackEvent& crack) {
    (void)stats;

    // Keep the list bounded to what the shader supports.
    if ((int)m_cracks.size() >= MAX_CRACKS) return;

    m_cracks.push_back(crack);
    m_created = true;
}

void CrackSystem::addCrack(const VolcanoStats& stats,
                           const glm::vec2& posXZ,
                           float startTime,
                           float width,
                           float radius) {
    CrackEvent e;
    e.posXZ = posXZ;
    e.startTime = startTime;
    e.width = width;
    e.radius = radius;
    addCrack(stats, e);
}

void CrackSystem::addCrack(const VolcanoStats& stats,
                           const glm::vec2& posXZ,
                           float startTime,
                           float width,
                           float radius,
                           const glm::vec2& dirXZ,
                           float halfLen) {
    CrackEvent e;
    e.posXZ = posXZ;
    e.startTime = startTime;
    e.width = width;
    e.radius = radius;
    e.dirXZ = dirXZ;
    e.halfLen = halfLen;
    addCrack(stats, e);
}

glm::vec2 CrackSystem::randomNearVolcanoXZ(const VolcanoStats& s) {
    // Bias cracks near the volcano base instead of the crater rim.
    // Use an annulus around baseRadius (with some thickness).
    const float rMin = s.baseRadius * 0.70f;
    const float rMax = s.baseRadius * 1.05f;

    const float ang = randRange(0.0f, 6.28318530718f);
    const float u = randRange(0.0f, 1.0f);
    const float r = std::sqrt(u) * (rMax - rMin) + rMin;

    return s.craterCenter + glm::vec2(std::cos(ang), std::sin(ang)) * r;
}

void CrackSystem::createCracksNearVolcano(const VolcanoStats& stats,
                                         int count,
                                         float nowTime) {
    m_cracks.clear();

    count = std::max(0, std::min(count, MAX_CRACKS));
    m_cracks.reserve((size_t)count);

    for (int i = 0; i < count; ++i) {
        CrackEvent e;
        e.posXZ = randomNearVolcanoXZ(stats);
        e.startTime = nowTime + randRange(0.0f, 0.35f);
        e.width = randRange(3.5f, 8.0f);
        e.radius = randRange(stats.craterRadius * 0.35f, stats.craterRadius * 1.1f);

        // NEW: random fissure direction + length
        float ang = randRange(0.0f, 6.28318530718f);
        e.dirXZ = glm::vec2(std::cos(ang), std::sin(ang));
        e.halfLen = randRange(55.0f, 140.0f);

        m_cracks.push_back(e);
    }

    m_created = true;
}

void CrackSystem::uploadToVolcanoShader(GLuint program) const {
    const int count = (int)std::min<size_t>(m_cracks.size(), MAX_CRACKS);

    const GLint locCount = glGetUniformLocation(program, "uCrackCount");
    if (locCount != -1) glUniform1i(locCount, count);

    if (count <= 0) return;

    std::vector<glm::vec2> pos(count);
    std::vector<float> startT(count);
    std::vector<float> width(count);
    std::vector<float> radius(count);

    for (int i = 0; i < count; ++i) {
        pos[i] = m_cracks[i].posXZ;
        startT[i] = m_cracks[i].startTime;
        width[i] = m_cracks[i].width;
        radius[i] = m_cracks[i].radius;
    }

    const GLint locPos = glGetUniformLocation(program, "uCrackPosXZ");
    if (locPos != -1) glUniform2fv(locPos, count, &pos[0].x);

    const GLint locStart = glGetUniformLocation(program, "uCrackStartTime");
    if (locStart != -1) glUniform1fv(locStart, count, startT.data());

    const GLint locWidth = glGetUniformLocation(program, "uCrackWidth");
    if (locWidth != -1) glUniform1fv(locWidth, count, width.data());

    const GLint locRadius = glGetUniformLocation(program, "uCrackRadius");
    if (locRadius != -1) glUniform1fv(locRadius, count, radius.data());

    // NEW: upload line parameters
    std::vector<glm::vec2> dir(count);
    std::vector<float> halfLen(count);
    for (int i = 0; i < count; ++i) {
        dir[i] = m_cracks[i].dirXZ;
        halfLen[i] = m_cracks[i].halfLen;
    }

    const GLint locDir = glGetUniformLocation(program, "uCrackDirXZ");
    if (locDir != -1 && count > 0) glUniform2fv(locDir, count, &dir[0].x);

    const GLint locHalf = glGetUniformLocation(program, "uCrackHalfLen");
    if (locHalf != -1 && count > 0) glUniform1fv(locHalf, count, halfLen.data());
}

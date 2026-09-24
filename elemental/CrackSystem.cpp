#include "CrackSystem.h"

#include <algorithm>

// Max cracks supported by the shader uniform arrays.
static constexpr int MAX_CRACKS = 8;

CrackSystem::CrackSystem() = default;

void CrackSystem::reset() {
    m_cracks.clear();
    m_uniformsDirty = true;
}

void CrackSystem::addCrack(const glm::vec2& posXZ,
                           float startTime,
                           float width,
                           const glm::vec2& dirXZ,
                           float halfLen) {
    if (static_cast<int>(m_cracks.size()) >= MAX_CRACKS) return;

    CrackEvent e;
    e.posXZ = posXZ;
    e.startTime = startTime;
    e.width = width;
    e.dirXZ = dirXZ;
    e.halfLen = halfLen;
    m_cracks.push_back(e);
    m_uniformsDirty = true;
}

void CrackSystem::cacheUniformLocations(GLuint program) {
    if (m_uniformProgram == program) return;

    m_uniformProgram = program;
    m_countLocation = glGetUniformLocation(program, "uCrackCount");
    m_positionLocation = glGetUniformLocation(program, "uCrackPosXZ[0]");
    m_startTimeLocation = glGetUniformLocation(program, "uCrackStartTime[0]");
    m_widthLocation = glGetUniformLocation(program, "uCrackWidth[0]");
    m_directionLocation = glGetUniformLocation(program, "uCrackDirXZ[0]");
    m_halfLengthLocation = glGetUniformLocation(program, "uCrackHalfLen[0]");
}

void CrackSystem::uploadToVolcanoShader(GLuint program) {
    const bool programChanged = m_uniformProgram != program;
    cacheUniformLocations(program);
    if (!m_uniformsDirty && !programChanged) return;

    const int count = (int)std::min<size_t>(m_cracks.size(), MAX_CRACKS);

    if (m_countLocation != -1) glUniform1i(m_countLocation, count);

    if (count <= 0) {
        m_uniformsDirty = false;
        return;
    }

    std::vector<glm::vec2> pos(count);
    std::vector<float> startT(count);
    std::vector<float> width(count);

    for (int i = 0; i < count; ++i) {
        pos[i] = m_cracks[i].posXZ;
        startT[i] = m_cracks[i].startTime;
        width[i] = m_cracks[i].width;
    }

    if (m_positionLocation != -1) glUniform2fv(m_positionLocation, count, &pos[0].x);
    if (m_startTimeLocation != -1) glUniform1fv(m_startTimeLocation, count, startT.data());
    if (m_widthLocation != -1) glUniform1fv(m_widthLocation, count, width.data());

    // Upload the oriented-line parameters used by the fissure distance field.
    std::vector<glm::vec2> dir(count);
    std::vector<float> halfLen(count);
    for (int i = 0; i < count; ++i) {
        dir[i] = m_cracks[i].dirXZ;
        halfLen[i] = m_cracks[i].halfLen;
    }

    if (m_directionLocation != -1) glUniform2fv(m_directionLocation, count, &dir[0].x);
    if (m_halfLengthLocation != -1) glUniform1fv(m_halfLengthLocation, count, halfLen.data());
    m_uniformsDirty = false;
}

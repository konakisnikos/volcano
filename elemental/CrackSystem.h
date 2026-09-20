#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

// Small CPU-side crack event system that stores the scripted fissures and
// uploads their parameters to the terrain shader.
class CrackSystem {
public:
    CrackSystem();

    void reset();

    void addCrack(const glm::vec2& posXZ,
                  float startTime,
                  float width,
                  float radius,
                  const glm::vec2& dirXZ,
                  float halfLen);

    // Upload uniforms to the given shader program.
    void uploadToVolcanoShader(GLuint volcanoShaderProgram);

private:
    struct CrackEvent {
        glm::vec2 posXZ{0.0f};
        float startTime = 0.0f;
        float width = 6.0f;
        float radius = 80.0f;
        glm::vec2 dirXZ{1.0f, 0.0f};
        float halfLen = 80.0f;
    };

    std::vector<CrackEvent> m_cracks;
    bool m_uniformsDirty = true;
    GLuint m_uniformProgram = 0;
    GLint m_countLocation = -1;
    GLint m_positionLocation = -1;
    GLint m_startTimeLocation = -1;
    GLint m_widthLocation = -1;
    GLint m_radiusLocation = -1;
    GLint m_directionLocation = -1;
    GLint m_halfLengthLocation = -1;

    void cacheUniformLocations(GLuint program);
};

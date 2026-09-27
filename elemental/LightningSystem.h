#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

class LightningSystem {
public:
    LightningSystem();
    ~LightningSystem();

    void trigger(float simulationTime, const glm::vec3& cloudPosition,
                 const glm::vec3& strikePosition);
    void update(float simulationTime, float simulationDelta);
    void reset();
    void draw(const glm::mat4& view, const glm::mat4& projection,
              float bloomStrength);
    bool consumeImpact();
    bool isActive() const { return m_active; }
    float flashStrength() const { return m_flashStrength; }
    glm::vec3 strikePosition() const { return m_strikePosition; }

private:
    struct BranchPath {
        int appearAfterSegment;
        std::vector<glm::vec3> points;
    };

    void buildBoltGeometry();
    void resizeBloom(int width, int height);

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_shader;
    GLint m_vLoc;
    GLint m_pLoc;
    GLint m_colorLoc;
    GLuint m_bloomFbos[3];
    GLuint m_bloomTextures[3];
    GLuint m_fullscreenVao;
    GLuint m_blurShader;
    GLuint m_compositeShader;
    GLint m_blurSourceLoc;
    GLint m_blurDirectionLoc;
    GLint m_compositeSourceLoc;
    GLint m_compositeStrengthLoc;
    int m_bloomWidth;
    int m_bloomHeight;
    bool m_bloomReady;
    bool m_active;
    bool m_impactPending;
    bool m_impactCreated;
    float m_strikeStartTime;
    float m_visualAge;
    float m_flashStrength;
    glm::vec3 m_cloudPosition;
    glm::vec3 m_strikePosition;
    std::vector<glm::vec3> m_mainPoints;
    std::vector<BranchPath> m_branches;
    std::vector<glm::vec3> m_vertices;
};

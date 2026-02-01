#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct VolcanoStats;

// Small CPU-side crack event system that:
// 1) Chooses a few crack seed points near the volcano.
// 2) Uploads crack parameters to the Volcano shader as uniforms.
class CrackSystem {
public:
    struct CrackEvent {
        glm::vec2 posXZ{0.0f};
        float startTime = 0.0f;
        float width = 6.0f;
        float radius = 80.0f;

        // oriented line fissure
        glm::vec2 dirXZ{1.0f, 0.0f};
        float halfLen = 80.0f;
    };

    CrackSystem();

    void reset();

    // Manually add a crack at a chosen world-space XZ position.
    void addCrack(const VolcanoStats& stats, const CrackEvent& crack);

    // Convenience helpers.
    void addCrack(const VolcanoStats& stats,
                  const glm::vec2& posXZ,
                  float startTime,
                  float width,
                  float radius);

    void addCrack(const VolcanoStats& stats,
                  const glm::vec2& posXZ,
                  float startTime,
                  float width,
                  float radius,
                  const glm::vec2& dirXZ,
                  float halfLen);

    // Create N cracks near the volcano crater/base.
    void createCracksNearVolcano(const VolcanoStats& stats,
                                 int count,
                                 float nowTime);

    // Upload uniforms to the given shader program.
    void uploadToVolcanoShader(GLuint volcanoShaderProgram) const;

    bool created() const { return m_created; }

private:
    // NOTE: keep compile-time constant in the .cpp to avoid linker issues on some toolchains.

    std::vector<CrackEvent> m_cracks;
    bool m_created = false;

    glm::vec2 randomNearVolcanoXZ(const VolcanoStats& stats);
    static float randRange(float a, float b);
};

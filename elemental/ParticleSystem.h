#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H


#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec4 color{1.0f};
    float life = 0.0f;      // seconds remaining (0 = dead)
    float size = 1.0f;      // current size in world units
    float startSize = 1.0f; // size at birth
    float rot = 0.0f;       // rotation in radians
};

struct ParticleEffectConfig {
    // ---- emission ----
    float spawnRate = 140.0f;     // particles/sec
    float lifetime  = 10.0f;      // seconds

    // Spawn disk in XZ plane around emitterPos (your current behavior)
    float emitterRadius = 5.0f;   // world units

    // ---- initial appearance ----
    glm::vec4 startColor = glm::vec4(0.78f, 0.80f, 0.88f, 1.0f);

    float startSizeMin = 6.0f;
    float startSizeMax = 12.0f;
    float endSizeMul   = 2.8f;

    // ---- initial velocity distribution (matches your current logic) ----
    float upSpeedMin = 10.0f;
    float upSpeedMax = 22.0f;

    float spreadMin  = 1.0f;
    float spreadMax  = 5.0f;

    // ---- motion ----
    float dragPerSec = 0.92f;               // 0..1
    glm::vec3 wind   = glm::vec3(1.5f, 0.0f, 0.8f);

    // NEW: physics toggles (lets you use same system for smoke AND stone)
    bool  useGravity = false;
    float gravity = 18.0f;          // positive value (m/s^2 style)
    bool  useBuoyancy = true;       // keep your current smoke lift
    float buoyancyBase = 6.0f;
    float buoyancyAgeBoost = 14.0f;

    // NEW: crude ground collision (optional)
    bool  collideWithPlane = false;
    float groundY = 0.0f;
    float bounce = 0.25f;           // 0 = no bounce, 1 = perfect
    float groundFriction = 0.6f;    // 0..1 multiplier on xz on bounce
};


class ParticleSystem {
public:
    explicit ParticleSystem(int maxParticles);

    void SetConfig(const ParticleEffectConfig& config);
    void Update(float dt, const glm::vec3& emitterPos);
    void Draw(GLuint shaderProgram);

private:
    struct InstanceData {
        glm::vec3 pos;
        glm::vec4 color;
        float size;
        float rot;
    };


    int maxParticles;
    int lastUsed = 0;

    std::vector<Particle> pool;

    // new: per-system state + config
    ParticleEffectConfig cfg{};
    float spawnAcc = 0.0f; // IMPORTANT: not static (supports multiple systems)

    GLuint vao = 0;
    GLuint vboQuad = 0;
    GLuint vboInstance = 0;

    void InitRenderData();
    int FindUnused();

};

#endif // PARTICLE_SYSTEM_H

#include "ParticleSystem.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdlib>

static const float TWO_PI = 6.28318530718f;

static float frand01() {
    return (float)(rand() % 10000) / 9999.0f;
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

ParticleSystem::ParticleSystem(int maxParticles)
    : maxParticles(maxParticles) {
    pool.resize(maxParticles);
    InitRenderData();
}

void ParticleSystem::SetConfig(const ParticleEffectConfig& config) {
    cfg = config;
}

void ParticleSystem::Update(float dt, const glm::vec3& emitterPos) {
    if (dt <= 0.0f) return;

    // ---- 1) Spawn FPS-independently ----
    spawnAcc += cfg.spawnRate * dt;
    int toSpawn = (int)spawnAcc;
    spawnAcc -= toSpawn;

    for (int i = 0; i < toSpawn; i++) {
        int id = FindUnused();
        if (id < 0) break;

        Particle& p = pool[id];

        p.life = cfg.lifetime;
        // Spawn in a disk around the emitter (crater opening)
        float theta  = frand01() * TWO_PI;
        float radius = std::sqrt(frand01()) * cfg.emitterRadius; // sqrt -> uniform disk
        float ox = std::cos(theta) * radius;
        float oz = std::sin(theta) * radius;

        p.position = emitterPos + glm::vec3(ox, 0.0f, oz);

        float r = frand01();

        // start size tuned for craterRadius ~ 40
        p.startSize = lerp(cfg.startSizeMin, cfg.startSizeMax, r);
        p.size = p.startSize;
        p.rot = frand01() * TWO_PI;

        // velocity
        float spread = lerp(cfg.spreadMin, cfg.spreadMax, frand01());
        float vx = (frand01() * 2.0f - 1.0f) * spread;
        float vz = (frand01() * 2.0f - 1.0f) * spread;
        float vy = lerp(cfg.upSpeedMin, cfg.upSpeedMax, frand01());
        p.velocity = glm::vec3(vx, vy, vz);

        // effect-configured start color
        p.color = cfg.startColor;
    }

    // ---- 2) Update alive particles ----
    // drag factor per frame (dt-safe)
    float drag = std::pow(cfg.dragPerSec, dt); // e.g. 0.85^dt

    for (auto& p : pool) {
        if (p.life <= 0.0f) continue;

        p.life -= dt;
        if (p.life <= 0.0f) {
            p.life = 0.0f;
            continue;
        }

        float age01 = 1.0f - (p.life / cfg.lifetime);

         // NEW: gravity (stone)
        if (cfg.useGravity) {
            p.velocity.y -= cfg.gravity * dt;
        }

        // buoyancy: stronger as smoke ages
        if (cfg.useBuoyancy) {
            p.velocity.y += (cfg.buoyancyBase + cfg.buoyancyAgeBoost * age01) * dt;

            float heightLift = glm::clamp(p.position.y * 0.01f, 0.0f, 3.0f);
            p.velocity.y += heightLift * dt;
        }

        // gentle wind (configurable)
        p.velocity += cfg.wind * dt;

        // drag
        p.velocity *= drag;

        // integrate
        p.position += p.velocity * dt;

        // NEW: simple ground collision (plane)
        if (cfg.collideWithPlane && p.position.y < cfg.groundY) {
            p.position.y = cfg.groundY;

            if (p.velocity.y < 0.0f) {
                p.velocity.y = -p.velocity.y * cfg.bounce;
                p.velocity.x *= cfg.groundFriction;
                p.velocity.z *= cfg.groundFriction;

                // if very slow after impact, kill early
                if (std::abs(p.velocity.y) < 0.6f) {
                    p.life = 0.0f;
                }
            }
        }

        // size curve: ease-out growth (fast early, slower later)
        float grow = 1.0f - std::pow(1.0f - age01, 3.0f);
        p.size = p.startSize * lerp(1.0f, cfg.endSizeMul, grow);

        // alpha curve: hold then fade
        float fade = 1.0f - age01;
        p.color.a = fade * fade; // smooth, non-linear
    }
}

void ParticleSystem::Draw(GLuint shaderProgram) {
    std::vector<InstanceData> instances;
    instances.reserve(maxParticles);

    for (const auto& p : pool) {
        if (p.life > 0.0f) {
            instances.push_back({p.position, p.color, p.size, p.rot});
        }
    }

    if (instances.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, vboInstance);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    instances.size() * sizeof(InstanceData),
                    instances.data());

    glBindVertexArray(vao);
    glUseProgram(shaderProgram);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)instances.size());
    glBindVertexArray(0);
}

void ParticleSystem::InitRenderData() {
    // A 2-triangle quad centered at origin (XY)
    float quad[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,

        -0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboQuad);
    glGenBuffers(1, &vboInstance);

    glBindVertexArray(vao);

    // quad positions (location 0)
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, vboInstance);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(InstanceData), nullptr, GL_STREAM_DRAW);

    // pos (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, pos));
    glVertexAttribDivisor(1, 1);

    // color (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(2, 1);

    // size (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, size));
    glVertexAttribDivisor(3, 1);

    // rot (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, rot));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

int ParticleSystem::FindUnused() {
    // search from last used
    for (int i = lastUsed; i < maxParticles; i++) {
        if (pool[i].life <= 0.0f) {
            lastUsed = i;
            return i;
        }
    }
    // wrap around
    for (int i = 0; i < lastUsed; i++) {
        if (pool[i].life <= 0.0f) {
            lastUsed = i;
            return i;
        }
    }
    return -1; // pool full => skip spawning this frame
}

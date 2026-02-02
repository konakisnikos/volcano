#include "SmokeEmitter.h"
#include <iostream>
#include <algorithm>
#include <cmath>

SmokeEmitter::SmokeEmitter(Drawable* _model,
                           int number,
                           glm::vec3 emitterPosition,
                           float spawnRadius): IntParticleEmitter(_model, number) {
    emitter_pos = emitterPosition;
    radius = spawnRadius;

    // Initialize all particles immediately so we don't see discrete spawn batches.
    active_particles = number_of_particles;
    for (int i = 0; i < active_particles; ++i) {
        createNewParticle(i);
    }
}

void SmokeEmitter::updateParticles(float time, float dt, glm::vec3 camera_pos) {

    for (int i = 0; i < active_particles; ++i) {
        particleAttributes& particle = p_attributes[i];

        // Respawn when it has risen above the plume height.
        // (This does NOT stop it at a hard cap; it just cycles particles to keep a stable plume.)
        if (particle.position.y > emitter_pos.y + plumeHeight) {
            createNewParticle(i);
            continue;
        }

        // Age / lifetime in [0,1]
        // Lower value = longer lifetime = particles can reach higher.
        particle.life -= dt * 0.12f;

        if (particle.life <= 0.0f) {
            createNewParticle(i);
            continue;
        }



        // Base upward acceleration (buoyancy) + mild gravity and damping
        glm::vec3 buoyancy(0.0f, 1.5f, 0.0f);
        glm::vec3 gravity(0.0f, -0.3f, 0.0f);
        glm::vec3 accel = buoyancy + gravity;

        // As smoke rises, make it spread outward more.
        float height01 = (particle.position.y - emitter_pos.y) / glm::max(1.0f, plumeHeight);
        height01 = std::max(0.0f, std::min(1.0f, height01));

        glm::vec3 radial = particle.position - emitter_pos;
        radial.y = 0.0f;
        float radialLen = glm::length(radial);
        if (radialLen > 1e-5f) radial /= radialLen;
        else radial = glm::vec3(0.0f);

        // Small lateral noise + radial push; both increase with height.
        float noiseStrength = 10.0f + 7.6f * height01;
        float spreadStrength = 2.0f + 6.2f * height01;
        glm::vec3 wind(
            (RAND - 0.5f) * noiseStrength,
            0.0f,
            (RAND - 0.5f) * noiseStrength
        );
        accel += wind + radial * spreadStrength;

        // Integrate motion
        particle.velocity += accel * dt;

        // Gentle drag so old smoke slows down
        //particle.velocity *= 0.985f;

        particle.position += particle.velocity * dt;

    // Slow rotation for texture variation
    particle.rot_angle += 20.0f * dt;

    // Make smoke puffs grow over time (mass == scale)
    float t = 1.0f - particle.life;                  // 0 → 1 over lifetime
    float minScale = 2.0f;
    float maxScale = 18.5f;
    particle.mass = minScale + (maxScale - minScale) * t;
        particle.dist_from_camera = length(particle.position - camera_pos);

        auto billRot = calculateBillboardRotationMatrix(particle.position, camera_pos);
        particle.rot_axis = glm::vec3(billRot.x, billRot.y, billRot.z);
        particle.rot_angle = glm::degrees(billRot.w);
    }
}


void SmokeEmitter::createNewParticle(int index) {
    particleAttributes& particle = p_attributes[index];

// Spawn in a small disk around the emitter (crater mouth etc.)
float x = (RAND - 0.5f) * 6.0f * radius;
float z = (RAND - 0.5f) * 6.0f * radius;
particle.position = emitter_pos + glm::vec3(x, 0.0f, z);

// Mostly upward velocity with a little sideways spread
particle.velocity = glm::vec3(
    0.0f,                            // sideways X (spread comes later with height)
    45.0f + RAND * 10.0f,            // upward
    0.0f                             // sideways Z
);

// Initial scale: small puff that will grow
particle.mass = 7.3f;

// Random spin axis
particle.rot_axis = glm::normalize(glm::vec3(
    1.0f - 2.0f * RAND,
    1.0f - 2.0f * RAND,
    1.0f - 2.0f * RAND
));

// We recompute accel each frame in updateParticles, so store zero here
particle.accel = glm::vec3(0.0f);

// Random initial rotation
particle.rot_angle = RAND * 360.0f;

// Full life in [0,1]
// Stagger life so particles don't all fade/respawn together (prevents visible "batches").
particle.life = 0.25f + 0.75f * RAND;

// Initialize distance (will be immediately updated in updateParticles)
particle.dist_from_camera = 0.0f;
}
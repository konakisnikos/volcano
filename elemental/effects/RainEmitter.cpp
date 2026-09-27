#include "RainEmitter.h"
#include <algorithm>
#include <cmath>

RainEmitter::RainEmitter(Drawable* _model,
                         int number,
                         glm::vec3 cloudPosition,
                         float groundLevel,
                         float spreadRadius): IntParticleEmitter(_model, number) {
    emitter_pos = cloudPosition;
    radius = spreadRadius;
    groundY = groundLevel;

    active_particles = number_of_particles;
    const float columnHeight = std::max(80.0f, emitter_pos.y - groundY);
    for (int i = 0; i < active_particles; ++i) {
        createNewParticle(i);
        // Fill the whole air column on the first frame. Later recycled drops
        // still begin at cloud level, so the shower remains physically legible.
        p_attributes[i].position.y -= RAND * columnHeight;
    }
}

void RainEmitter::createNewParticle(int index) {
    particleAttributes& particle = p_attributes[index];

    const float angle = RAND * 6.2831853f;
    const float radial = std::sqrt(RAND);
    float x = std::cos(angle) * radius * radial;
    float z = std::sin(angle) * radius * 0.62f * radial;
    float y = RAND * 35.0f;
    particle.position = emitter_pos + glm::vec3(x, y, z);

    // Fast fall plus a shared wind direction produces readable diagonal rain,
    // while small per-drop variation prevents one rigid curtain.
    particle.velocity = glm::vec3(16.0f + RAND * 10.0f,
                                  -255.0f - RAND * 85.0f,
                                  2.0f + RAND * 8.0f);
    particle.accel = glm::vec3(0.0f, -24.0f, 0.0f);

    particle.mass = 0.78f + RAND * 0.58f; // Y stretch is supplied by the shader
    particle.rot_axis = glm::vec3(0.0f, 1.0f, 0.0f);
    particle.rot_angle = 0.0f;

    particle.life = 0.58f + RAND * 0.22f;
    particle.dist_from_camera = 0.0f;
}

void RainEmitter::updateParticles(float time, float dt, glm::vec3 camera_pos) {
    for (int i = 0; i < active_particles; ++i) {
        particleAttributes& particle = p_attributes[i];

        particle.velocity += particle.accel * dt;
        particle.velocity.x += std::sin(time * 0.7f + particle.position.z * 0.018f)
                             * dt * 3.0f;
        particle.position += particle.velocity * dt;

        if (particle.position.y <= groundY) {
            createNewParticle(i);
            continue;
        }

        particle.dist_from_camera = length(particle.position - camera_pos);

        auto billRot = calculateBillboardRotationMatrix(particle.position, camera_pos);
        particle.rot_axis = glm::vec3(billRot.x, billRot.y, billRot.z);
        particle.rot_angle = glm::degrees(billRot.w);
    }
}

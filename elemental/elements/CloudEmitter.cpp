#include "CloudEmitter.h"
#include <algorithm>
#include <cmath>

CloudEmitter::CloudEmitter(Drawable* _model,
                           int number,
                           glm::vec3 centerPosition,
                           float spreadRadius,
                           float verticalSpreadValue,
                           float minPuffSizeValue,
                           float maxPuffSizeValue,
                           float driftSpeedValue,
                           float formationSeconds): IntParticleEmitter(_model, number) {
    emitter_pos = centerPosition;
    radius = spreadRadius;
    verticalSpread = verticalSpreadValue;
    minPuffSize = minPuffSizeValue;
    maxPuffSize = maxPuffSizeValue;
    driftSpeed = driftSpeedValue;
    formDuration = formationSeconds;
    use_sorting = true;

    active_particles = number_of_particles;
    for (int i = 0; i < active_particles; ++i) {
        createNewParticle(i);
    }
}

void CloudEmitter::createNewParticle(int index) {
    particleAttributes& particle = p_attributes[index];

    // A radial ellipse avoids the old rectangular cloud footprint. Small vertical
    // strata make overlapping puffs read as one layered cloud bank.
    const float angle = RAND * 6.2831853f;
    // Keep the storm core denser while leaving a broken, lighter outer edge.
    const float radial = std::pow(RAND, 0.70f);
    float x = std::cos(angle) * radius * radial;
    float z = std::sin(angle) * radius * 0.55f * radial;
    float y = (RAND - 0.5f) * verticalSpread
            + std::sin(angle * 2.0f) * verticalSpread * 0.12f;
    particle.position = emitter_pos + glm::vec3(x, y, z);

    // Gentle drift, no net rise/fall (clouds hover).
    particle.velocity = glm::vec3(
        driftSpeed * (0.35f + RAND * 0.65f),
        (RAND - 0.5f) * driftSpeed * 0.04f,
        (RAND - 0.5f) * driftSpeed * 0.25f
    );

    // Formation changes scale and opacity while drift keeps constant velocity.
    particle.accel = glm::vec3(0.0f);
    particle.targetScale = minPuffSize + RAND * (maxPuffSize - minPuffSize);
    particle.opacityLimit = 0.52f + RAND * 0.30f;
    particle.animationPhase = RAND * 6.2831853f;
    particle.mass = 0.0f; // grows toward targetScale as the cloud forms

    particle.rot_axis = glm::vec3(0.0f, 1.0f, 0.0f);
    particle.rot_angle = RAND * 360.0f;

    // Stagger so puffs don't all appear in lockstep.
    particle.life = RAND * particle.opacityLimit * 0.35f;

    particle.dist_from_camera = 0.0f;
}

void CloudEmitter::updateParticles(float time, float dt, glm::vec3 camera_pos) {
    for (int i = 0; i < active_particles; ++i) {
        particleAttributes& particle = p_attributes[i];

        float maxOpacity = particle.opacityLimit;
        particle.life = std::min(particle.life + dt / formDuration, maxOpacity);
        float growth = particle.life / maxOpacity; // 0..1 formation progress for this puff

        particle.position += particle.velocity * dt;
        particle.position.y += std::sin(time * 0.16f + particle.animationPhase) * dt * 0.18f;
        particle.mass = particle.targetScale * growth;

        particle.dist_from_camera = length(particle.position - camera_pos);

        auto billRot = calculateBillboardRotationMatrix(particle.position, camera_pos);
        particle.rot_axis = glm::vec3(billRot.x, billRot.y, billRot.z);
        particle.rot_angle = glm::degrees(billRot.w);
    }
}

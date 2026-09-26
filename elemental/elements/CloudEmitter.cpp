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

    // A broad, deep lower deck and a raised center form one cloud volume.
    // The curved envelope and small height variations keep the edges irregular.
    const float horizontalSample = RAND * 2.0f - 1.0f;
    const float x = std::copysign(
        radius * std::pow(std::abs(horizontalSample), 1.15f), horizontalSample);
    const float edge = std::abs(x) / radius;
    const float crown = std::pow(std::max(0.0f, 1.0f - edge * edge), 1.6f);
    const float ridge = 0.5f + 0.5f * std::sin(x * 0.026f + 0.7f)
                                     * std::sin(x * 0.013f - 1.0f);
    const float bottom = verticalSpread * (-0.30f + 0.035f * std::sin(x * 0.019f));
    const float top = verticalSpread * (-0.03f + 0.47f * crown
                                        + 0.08f * ridge * crown);
    const float y = bottom + (top - bottom) * std::pow(RAND, 1.4f)
                  + (RAND - 0.5f) * 12.0f;
    const float depthRadius = radius * 0.45f
                            * std::sqrt(std::max(0.12f, 1.0f - edge * edge));
    const float z = (RAND * 2.0f - 1.0f) * depthRadius;
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
    particle.opacityLimit = 0.70f + RAND * 0.25f;
    particle.animationPhase = RAND * 6.2831853f;
    particle.mass = 0.0f; // grows toward targetScale as the cloud forms

    particle.rot_axis = glm::vec3(0.0f, 1.0f, 0.0f);
    particle.rot_angle = RAND * 360.0f;

    // Start near the smoke column and let the wider cloud appear afterward.
    const float outwardDelay = 2.2f * std::pow(std::min(1.0f, std::abs(x) / radius), 1.1f);
    const float delaySeconds = 0.15f + outwardDelay + 0.45f * RAND;
    particle.life = -delaySeconds / formDuration;

    particle.dist_from_camera = 0.0f;
}

void CloudEmitter::updateParticles(float time, float dt, glm::vec3 camera_pos) {
    for (int i = 0; i < active_particles; ++i) {
        particleAttributes& particle = p_attributes[i];

        float maxOpacity = particle.opacityLimit;
        particle.life = std::min(particle.life + dt / formDuration, maxOpacity);
        float growth = glm::smoothstep(0.0f, 1.0f,
                                       std::max(0.0f, particle.life / maxOpacity));

        particle.position += particle.velocity * dt;
        particle.position.y += std::sin(time * 0.16f + particle.animationPhase) * dt * 0.18f;
        particle.mass = particle.targetScale * std::sqrt(growth);

        particle.dist_from_camera = length(particle.position - camera_pos);

        auto billRot = calculateBillboardRotationMatrix(particle.position, camera_pos);
        particle.rot_axis = glm::vec3(billRot.x, billRot.y, billRot.z);
        particle.rot_angle = glm::degrees(billRot.w);
    }
}

#include "CloudEmitter.h"
#include <algorithm>
#include <cmath>
#include <SOIL.h>

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

    int channels = 0;
    unsigned char* pixels = SOIL_load_image(
        ELEMENTAL_ASSET_DIR "/storm_cloud_mass.png",
        &m_guideWidth, &m_guideHeight, &channels, SOIL_LOAD_RGBA);
    if (pixels) {
        m_guidePixels.assign(pixels, pixels + m_guideWidth * m_guideHeight * 4);
        glGenTextures(1, &m_guideTexture);
        glBindTexture(GL_TEXTURE_2D, m_guideTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_guideWidth, m_guideHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        SOIL_free_image_data(pixels);
    }

    active_particles = number_of_particles;
    for (int i = 0; i < active_particles; ++i) {
        createNewParticle(i);
    }
}

CloudEmitter::~CloudEmitter() {
    if (m_guideTexture) glDeleteTextures(1, &m_guideTexture);
}

void CloudEmitter::createNewParticle(int index) {
    particleAttributes& particle = p_attributes[index];

    float x, y, z;
    float density = 1.0f;
    if (!m_guidePixels.empty()) {
        // The image alpha is a density map: opaque parts receive more puffs,
        // while the transparent silhouette receives none.
        int pixelX = m_guideWidth / 2;
        int pixelY = m_guideHeight / 2;
        for (int attempt = 0; attempt < 100; ++attempt) {
            pixelX = std::min(int(RAND * m_guideWidth), m_guideWidth - 1);
            pixelY = std::min(int(RAND * m_guideHeight), m_guideHeight - 1);
            density = m_guidePixels[(pixelY * m_guideWidth + pixelX) * 4 + 3] / 255.0f;
            if (RAND < density * density) break;
        }
        x = (float(pixelX) / m_guideWidth - 0.5f) * radius * 2.0f;
        y = (0.5f - float(pixelY) / m_guideHeight) * verticalSpread;
        z = (RAND - 0.5f) * 95.0f;
    } else {
        const float angle = RAND * 6.2831853f;
        const float radial = std::pow(RAND, 0.70f);
        x = std::cos(angle) * radius * radial;
        z = std::sin(angle) * radius * 0.55f * radial;
        y = (RAND - 0.5f) * verticalSpread;
    }
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
    particle.opacityLimit = (0.62f + RAND * 0.26f) * std::max(0.35f, density);
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

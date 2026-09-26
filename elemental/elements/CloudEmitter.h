#ifndef CLOUDEMITTER_H
#define CLOUDEMITTER_H

#include <elemental/IntParticleEmitter.h>

// Cloud particles grow in place before drifting.
// Unlike SmokeEmitter, particles don't rise and die/respawn — they spawn once,
// drift gently, and grow from nothing into a steady puff as the cloud "forms".
// life starts below zero for a staggered appearance and rises to opacityLimit;
// SmokeEmitter instead counts life down to respawn.
class CloudEmitter : public IntParticleEmitter {
    public:
        CloudEmitter(Drawable* _model,
                      int number,
                      glm::vec3 centerPosition,
                      float spreadRadius = 150.0f,
                      float verticalSpread = 40.0f,
                      float minPuffSize = 18.0f,
                      float maxPuffSize = 40.0f,
                      float driftSpeed = 3.0f,
                      float formationSeconds = 6.0f);

        float radius = 150.0f;
        float verticalSpread = 40.0f;
        float minPuffSize = 18.0f;
        float maxPuffSize = 40.0f;
        float driftSpeed = 3.0f;

        // How long (seconds) it takes a particle to grow from spawn to full size/opacity.
        float formDuration = 6.0f;

        int active_particles = 0;
        void createNewParticle(int index) override;
        void updateParticles(float time, float dt, glm::vec3 camera_pos = glm::vec3(0, 0, 0)) override;

};

#endif // CLOUDEMITTER_H

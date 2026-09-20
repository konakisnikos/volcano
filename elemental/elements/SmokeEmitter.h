#ifndef SMOKEEMITTER_H
#define SMOKEEMITTER_H

#include <elemental/IntParticleEmitter.h>

class SmokeEmitter : public IntParticleEmitter {
    public:
        // emitterPosition sets the world-space origin of this emitter
        // spawnRadius controls how far from that origin new particles can spawn
        SmokeEmitter(Drawable* _model,
                     int number,
                     glm::vec3 emitterPosition,
                     float spawnRadius = 10.0f,
                     float minParticleScale = 2.0f,
                     float maxParticleScale = 12.0f,
                     float minimumRiseSpeed = 45.0f,
                     float riseSpeedVariation = 10.0f,
                     float lifeDecay = 0.12f);

        //data member for collision checking
        float radius = 10.0f;

        // How high (in world units) smoke can rise above emitter_pos before it stops rising.
        // Increase this to make the plume reach higher.
        float plumeHeight = 1288800.0f;

        // Multiplier for sideways turbulence and slow texture rotation.
        // Values below one produce a calm fumarolic/steam plume.
        float turbulence = 1.0f;

        int active_particles = 0; //number of particles that have been instantiated
        void createNewParticle(int index) override;
        void updateParticles(float time, float dt, glm::vec3 camera_pos = glm::vec3(0, 0, 0)) override;

        // Move a continuous source (the lava-cooling front) and recycle a small,
        // time-based subset of particles there. Existing particles stay behind,
        // producing a plume trail instead of teleporting with the emitter.
        void followMovingSource(const glm::vec3& newPosition,
                                float dt,
                                float respawnsPerSecond);

    private:
        float m_minParticleScale;
        float m_maxParticleScale;
        float m_minimumRiseSpeed;
        float m_riseSpeedVariation;
        float m_lifeDecay;
        float m_followRespawnAccumulator = 0.0f;
        int m_followRespawnCursor = 0;

};



#endif // SMOKEEMITTER_H

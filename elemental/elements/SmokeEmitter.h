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
                     float spawnRadius = 10.0f);

        //data member for collision checking
        float radius = 10.0f;

        // How high (in world units) smoke can rise above emitter_pos before it stops rising.
        // Increase this to make the plume reach higher.
        float plumeHeight = 1288800.0f;

        int active_particles = 0; //number of particles that have been instantiated
        void createNewParticle(int index) override;
        void updateParticles(float time, float dt, glm::vec3 camera_pos = glm::vec3(0, 0, 0)) override;

};



#endif // SMOKEEMITTER_H
#ifndef RAINEMITTER_H
#define RAINEMITTER_H

#include <elemental/IntParticleEmitter.h>

// Drops recycle at groundY to maintain a continuous shower.
class RainEmitter : public IntParticleEmitter {
    public:
        // cloudPosition is the spawn area (XZ + altitude); groundLevel is the Y at
        // which a falling drop recycles back to the top.
        RainEmitter(Drawable* _model,
                    int number,
                    glm::vec3 cloudPosition,
                    float groundLevel,
                    float spreadRadius = 150.0f);

        float radius = 150.0f;
        float groundY = 0.0f;

        int active_particles = 0;
        void createNewParticle(int index) override;
        void updateParticles(float time, float dt, glm::vec3 camera_pos = glm::vec3(0, 0, 0)) override;
};

#endif // RAINEMITTER_H

#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <common/model.h>

//#define USE_PARALLEL_TRANSFORM

//Gives a random number between 0 and 1
#define RAND ((float) rand()) / (float) RAND_MAX

struct particleAttributes {
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 rot_axis = glm::vec3(0, 1, 0);
    float rot_angle = 0.0f; //degrees
    glm::vec3 accel = glm::vec3(0, 0, 0);
    glm::vec3 velocity = glm::vec3(0, 0, 0);
    float life = 0.0f;
    // The original Lab 8 particle interface calls this value mass. In this
    // renderer it is passed to the vertex shader as the billboard scale.
    float mass = 0.0f;

    // Visual controls used only by cloud puffs. Keeping them named avoids
    // hiding non-physics data in acceleration components.
    float targetScale = 0.0f;
    float opacityLimit = 1.0f;
    float animationPhase = 0.0f;

    float dist_from_camera = 0.0f; //In case you want to do depth sorting
    bool operator < (const particleAttributes& p) const
    {
        return dist_from_camera < p.dist_from_camera;
    }
    bool operator > (const particleAttributes& p) const
    {
        return dist_from_camera > p.dist_from_camera;
    }

};

// All per-instance values share one VBO. Keeping them interleaved turns the old
// eight buffer orphan/upload calls per emitter into one upload per frame, which
// avoids visible stalls in OpenGL drivers that translate commands to Metal.
struct ParticleInstanceData {
    glm::mat4 translation = glm::mat4(1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    float scale = 1.0f;
    float life = 0.0f;
};


//ParticleEmitterInt is an interface class. Emitter classes must derive from this one and implement the updateParticles method
class IntParticleEmitter
{
public:
    GLuint emitterVAO;
    int number_of_particles;

    std::vector<particleAttributes> p_attributes;

    bool use_rotations = true;
    bool use_sorting = false;


    glm::vec3 emitter_pos; //the origin of the emitter

    IntParticleEmitter(Drawable* _model, int number);
    virtual ~IntParticleEmitter();
    void changeParticleNumber(int new_number);

    void renderParticles(int time = 0);
    virtual void updateParticles(float time, float dt, glm::vec3 camera_pos) = 0;
    virtual void createNewParticle(int index) = 0;

    glm::vec4 calculateBillboardRotationMatrix(glm::vec3 particle_pos, glm::vec3 camera_pos);


private:
    std::vector<ParticleInstanceData> instanceData;

    Drawable* model;
    void configureVAO();
    void bindAndUpdateBuffers();
    GLuint instanceBuffer;
};

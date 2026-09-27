#include "IntParticleEmitter.h"
#include <algorithm>
#include <cstddef>

IntParticleEmitter::IntParticleEmitter(Drawable* _model, int number) {
    model = _model;
    number_of_particles = number;
    emitter_pos = glm::vec3(0.0f, 0.0f, 0.0f);
    p_attributes.resize(number_of_particles, particleAttributes());

    instanceData.resize(number_of_particles);

    configureVAO();
}

IntParticleEmitter::~IntParticleEmitter() {
    glDeleteBuffers(1, &instanceBuffer);
    glDeleteVertexArrays(1, &emitterVAO);
}

void IntParticleEmitter::renderParticles(int time) {
    if (number_of_particles == 0) return;
    bindAndUpdateBuffers();
    // indices.size() is already the total index count (3 per triangle); do NOT multiply
    // by 3 again or glDrawElements reads past the element buffer and draws nothing.
    glDrawElementsInstanced(GL_TRIANGLES, model->indices.size(), GL_UNSIGNED_INT, 0, number_of_particles);
}

glm::vec4 IntParticleEmitter::calculateBillboardRotationMatrix(glm::vec3 particle_pos, glm::vec3 camera_pos)
{
    glm::vec3 dir = camera_pos - particle_pos;
    dir.y = 0;
    dir = glm::normalize(dir);

    glm::vec3 rot_axis = glm::cross(glm::vec3(0, 0, 1), dir);
    float rot_angle = glm::acos(glm::dot(glm::vec3(0, 0, 1), dir));

    return glm::vec4(rot_axis.x, rot_axis.y, rot_axis.z, rot_angle);
}

void IntParticleEmitter::bindAndUpdateBuffers()
{
    if (use_sorting) {
        // Standard alpha blending is order-dependent, so translucent puffs must
        // reach the GPU from farthest to nearest relative to the camera.
        std::sort(p_attributes.begin(), p_attributes.end(),
                  [](const particleAttributes& a, const particleAttributes& b) {
                      return a.dist_from_camera > b.dist_from_camera;
                  });
    }

    for (std::size_t i = 0; i < p_attributes.size(); ++i) {
        const particleAttributes& particle = p_attributes[i];
        ParticleInstanceData& instance = instanceData[i];
        instance.translation = glm::translate(glm::mat4(1.0f), particle.position);
        instance.rotation = use_rotations
            ? glm::rotate(glm::mat4(1.0f), glm::radians(particle.rot_angle),
                          particle.rot_axis)
            : glm::mat4(1.0f);
        instance.scale = particle.mass;
        instance.life = particle.life;
    }

    // One interleaved upload replaces four orphan + four sub-data operations.
    // Besides reducing API traffic, this prevents repeated GPU synchronization
    // observed in Apple's OpenGL-to-Metal driver.
    glBindVertexArray(emitterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instanceData.size() * sizeof(ParticleInstanceData)),
                 instanceData.data(), GL_STREAM_DRAW);
}

void IntParticleEmitter::changeParticleNumber(int new_number) {
    if (new_number == number_of_particles) return;

    number_of_particles = new_number;
    p_attributes.resize(number_of_particles, particleAttributes());
    instanceData.resize(number_of_particles);

}

void IntParticleEmitter::configureVAO()
{
    glGenVertexArrays(1, &emitterVAO);
    glBindVertexArray(emitterVAO);


    //We are using the model's buffer but since they are already in the GPU from the Drawable's constructor we just need to configure
    //our own VAO by using glVertexAttribPointer and glEnableVertexAttribArray but without sending any data with glBufferData.
    glBindBuffer(GL_ARRAY_BUFFER, model->verticesVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    if (model->indexedNormals.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, model->normalsVBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);
    }


    if (model->indexedUVS.size() != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, model->uvsVBO);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->elementVBO);

    // GLSL mat4 inputs consume four consecutive locations each. All attributes
    // use the same interleaved stride and offsets within ParticleInstanceData.
    glGenBuffers(1, &instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    const GLsizei stride = static_cast<GLsizei>(sizeof(ParticleInstanceData));
    const std::size_t translationOffset = offsetof(ParticleInstanceData, translation);
    const std::size_t rotationOffset = offsetof(ParticleInstanceData, rotation);

    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = 3 + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location, 4, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(translationOffset + column * sizeof(glm::vec4)));
        glVertexAttribDivisor(location, 1);
    }
    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = 7 + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location, 4, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(rotationOffset + column * sizeof(glm::vec4)));
        glVertexAttribDivisor(location, 1);
    }

    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(ParticleInstanceData, scale)));
    glVertexAttribDivisor(11, 1);

    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(ParticleInstanceData, life)));
    glVertexAttribDivisor(12, 1);

    glBindVertexArray(0);
}

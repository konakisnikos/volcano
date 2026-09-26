#ifndef SKYBOX_H
#define SKYBOX_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/model.h"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);
    ~Skybox();

    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
              float lightningFlash = 0.0f,
              float timeSeconds = 0.0f,
              const glm::vec3& moonDirection = glm::vec3(-0.4f, 0.65f, -0.65f),
              float calmProgress = 0.0f,
              float stormProgress = 0.0f);

private:
    Drawable *m_drawable;
    GLuint m_cubemapTexture;
    GLuint m_cloudTexture;
    GLuint m_shaderProgram;
    GLint m_viewLocation;
    GLint m_projectionLocation;
    GLint m_flashLocation;
    GLint m_timeLocation;
    GLint m_moonLocation;
    GLint m_calmLocation;
    GLint m_stormProgressLocation;

    void loadCubemap(const std::vector<std::string>& faces);
    void setupMesh();
};

#endif // SKYBOX_H

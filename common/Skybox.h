#ifndef SKYBOX_H
#define SKYBOX_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "model.h"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);
    ~Skybox();

    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

private:
    Drawable *m_drawable;
    GLuint m_cubemapTexture;
    GLuint m_shaderProgram;

    void loadCubemap(const std::vector<std::string>& faces);
    void setupMesh();
    GLuint loadShaders(const char* vertexPath, const char* fragmentPath);
};

#endif // SKYBOX_H

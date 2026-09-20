#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class Drawable;

// Draws many copies of one mesh with one OpenGL call.  Flowers contain several
// repeated spheres and cylinders, so per-instance model matrices and colours
// remove hundreds of otherwise identical draw calls without changing their look.
class InstancedPropRenderer {
public:
    explicit InstancedPropRenderer(Drawable* model);
    ~InstancedPropRenderer();

    void draw(const std::vector<glm::mat4>& modelMatrices,
              const std::vector<glm::vec3>& colors);

private:
    Drawable* m_model;
    GLuint m_vao = 0;
    GLuint m_matrixBuffer = 0;
    GLuint m_colorBuffer = 0;
};

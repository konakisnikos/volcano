#ifndef VOLCANO_H
#define VOLCANO_H

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "shader.h"

class Volcano {
public:
    Volcano(int gridSize, float maxTerrainWidth);
    ~Volcano();
    void Draw(GLuint shaderProgram);

private:
    unsigned int m_VAO, m_VBO_Pos, m_VBO_Norm, m_VBO_Tex, m_EBO;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    std::vector<unsigned int> m_indices;

    int m_gridSize;
    float m_width;
    float m_heightScale;

    void generateGeometry(); //creates vertices and indices
    void calculateNormals(); //for lighting
    void setupMesh(); //sets up VAO and VBOs
    float gaussian(float x, float y); //for volcano shape
    float applyRiverPath(); //modifies terrain for river
};

#endif // VOLCANO_H
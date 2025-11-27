#ifndef VOLCANO_H
#define VOLCANO_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "shader.h"
#include "model.h"


class Volcano {
public:
    Volcano(int gridSize, float maxTerrainWidth, float heightScale);
    ~Volcano();

    void Draw();

private:

    int m_gridSize;
    float m_width;
    float m_heightScale;

    Drawable* m_drawable;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    std::vector<float>     m_heightMap;  // grayscale heights from heightmap image

    void generateGeometry(); //creates vertices and indices
    void calculateNormals(); //for lighting
    //void applyRiverPath(); //modifies terrain for river
    void loadHeightMap(const std::string& path); //optional heightmap loading
    void useHeightMap(); //applies heightmap to m_positions
};

#endif // VOLCANO_H
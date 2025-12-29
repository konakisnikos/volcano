#ifndef VOLCANO_H
#define VOLCANO_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "common/shader.h"
#include "common/model.h"


struct VolcanoStats {
    float peakY;        // The top emission point
    float baseY;        // Where the slope ends
    float riverBedY;    // The bottom of the trench
    float riverWaterY;  // How high the river fills
    float centerZ;      // The Z coordinate of the volcano center
    float baseRadius;
    
    // Crater-specific data for lava plane
    float craterRadius;     // Horizontal radius of the crater
    float craterBottom;     // Y position of crater floor
    float craterTop;        // Y position of crater rim
    glm::vec2 craterCenter; // (X, Z) position of crater center
};


class Volcano {
public:
    Volcano(int gridSize, float maxTerrainWidth, float heightScale, glm::vec2 volcanoCenter = glm::vec2(0.0f, 0.0f));
    ~Volcano();

    void Draw();

    VolcanoStats getStats() const { return m_stats; }

private:

    int m_gridSize;
    float m_width;
    float m_heightScale;
    glm::vec2 m_volcanoCenter;

    Drawable* m_drawable;

    VolcanoStats m_stats;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    std::vector<float>     m_heightMap;  // grayscale heights from heightmap image
    std::vector<float>     m_riverMasks;
    std::vector<float>     m_distFromCenter;

    GLuint m_riverMaskBuffer;
    GLuint m_distBuffer;
    
    
    void setupExtraAttributes();
    void generateGeometry(); //creates vertices and indices
    void calculateNormals(); //for lighting
    void loadHeightMap(const std::string& path); //optional heightmap loading
    void useHeightMap(); //applies heightmap to m_positions
};

#endif // VOLCANO_H
#ifndef VOLCANO_H
#define VOLCANO_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
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

    // River channel extent (distance from craterCenter), for the domino chain:
    // lava/water travel from riverInnerRadius out to riverOuterRadius.
    float riverInnerRadius;
    float riverOuterRadius;
    glm::vec2 riverEndXZ;   // world-space (X, Z) of the far end of the river channel
    float riverEndY;        // approximate world-space Y at the river end (river bed height)
};


class Volcano {
public:
    Volcano(int gridSize, float maxTerrainWidth, float heightScale, glm::vec2 volcanoCenter = glm::vec2(0.0f, 0.0f));
    ~Volcano();

    void Draw();

    VolcanoStats getStats() const { return m_stats; }

    // Bilinear CPU lookup of the already-generated terrain. Particle sources use
    // it to stay on the river bed while moving; it does not regenerate geometry.
    float surfaceHeightAt(float worldX, float worldZ) const;

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
    std::vector<float>     m_surfaceHeights;
    std::vector<float>     m_riverMasks;
    std::vector<float>     m_grassMasks;
    std::vector<float>     m_distFromCenter;

    
    void generateGeometry(); //creates vertices and indices
    void calculateNormals(); //for lighting
    void loadHeightMap(const std::string& path); //optional heightmap loading
};

#endif // VOLCANO_H

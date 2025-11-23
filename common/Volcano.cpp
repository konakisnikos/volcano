#include "Volcano.h"
#include <iostream>

Volcano::Volcano(int gridSize, float maxTerrainWidth, float heightScale)
    : m_gridSize(gridSize), m_width(maxTerrainWidth), m_heightScale(heightScale), m_drawable(nullptr)
{
    generateGeometry();
    calculateNormals();
    m_drawable = new Drawable(m_positions, m_texCoords, m_normals);
    std::cout << "Volcano created with "
              << m_positions.size() << " vertices ("
              << m_positions.size() / 3 << " triangles)." << std::endl;
}

Volcano::~Volcano() {
    if (m_drawable) {
        delete m_drawable;
    }
}

void Volcano::applyRiverPath() {
    //leave empty for now

}

void Volcano::generateGeometry() {
    std::vector<glm::vec3> gridPositions;
    std::vector<glm::vec2> gridTexCoords;

    gridPositions.reserve(m_gridSize * m_gridSize);
    gridTexCoords.reserve(m_gridSize * m_gridSize);

    float step = m_width / (m_gridSize - 1);
    float halfWidth = m_width * 0.5f;

    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {
            float x = static_cast<float>(j) * step - halfWidth;
            float z = static_cast<float>(i) * step - halfWidth;
            float y = 0.0f; // simple 2D plane on the XZ axis

            gridPositions.emplace_back(x, y, z);
            gridTexCoords.emplace_back(
                static_cast<float>(j) / (m_gridSize - 1),
                static_cast<float>(i) / (m_gridSize - 1)
            );
        }
    }

    m_positions = gridPositions;
    m_texCoords = gridTexCoords;


    // Generate indices for triangle rendering
    std::vector<glm::vec3> rawPositions;
    std::vector<glm::vec2> rawTexCoords;

    for (int i=0; i<m_gridSize - 1; ++i) {
        for (int j=0; j<m_gridSize - 1; ++j) {
            int v0 = i * m_gridSize + j; //top-left
            int v1 = (i + 1) * m_gridSize + j; //bottom-left
            int v2 = v0 + 1; //top-right
            int v3 = v1 + 1; //bottom-right

            // First triangle V0, V1, V3
            rawPositions.push_back(m_positions[v0]);
            rawTexCoords.push_back(m_texCoords[v0]);
            rawPositions.push_back(m_positions[v1]);
            rawTexCoords.push_back(m_texCoords[v1]);
            rawPositions.push_back(m_positions[v3]);
            rawTexCoords.push_back(m_texCoords[v3]);

            // Second triangle V0, V3, V2
            rawPositions.push_back(m_positions[v0]);
            rawTexCoords.push_back(m_texCoords[v0]);
            rawPositions.push_back(m_positions[v3]);
            rawTexCoords.push_back(m_texCoords[v3]);
            rawPositions.push_back(m_positions[v2]);
            rawTexCoords.push_back(m_texCoords[v2]);

        }
    }

    m_positions = rawPositions;
    m_texCoords = rawTexCoords;
}

void Volcano::calculateNormals() {

    m_normals.clear();
    m_normals.resize(m_positions.size());

    for (size_t i=0; i<m_positions.size(); i+=3) {
        const glm::vec3& v1 = m_positions[i];
        const glm::vec3& v2 = m_positions[i+1];
        const glm::vec3& v3 = m_positions[i+2];

        glm::vec3 edge1 = v2 - v1;
        glm::vec3 edge2 = v3 - v1;

        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        m_normals[i] = normal;
        m_normals[i+1] = normal;
        m_normals[i+2] = normal;
    }
}


void Volcano::Draw() {
    if (m_drawable) {
        m_drawable->bind();
        m_drawable->draw();
    }
}
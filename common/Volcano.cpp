#include "Volcano.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstdint>

#define GAUSSIAN_SIGMA_SQ 15.0f

// Noise utilities (value + fractal noise) -------------------------------------------------
namespace {
    inline float smooth(float t) { return t * t * (3.0f - 2.0f * t); }
    inline float hash2(int x, int y) {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u; // large primes
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        // scale to [0,1]
        return (h & 0xFFFFFF) / float(0xFFFFFF);
    }
    float valueNoise(float x, float y) {
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        float tx = smooth(x - x0);
        float ty = smooth(y - y0);
        float v00 = hash2(x0, y0);
        float v10 = hash2(x1, y0);
        float v01 = hash2(x0, y1);
        float v11 = hash2(x1, y1);
        float vx0 = v00 + (v10 - v00) * tx;
        float vx1 = v01 + (v11 - v01) * tx;
        return vx0 + (vx1 - vx0) * ty; // [0,1]
    }
    float fractalNoise(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;
        for (int i = 0; i < octaves; ++i) {
            float n = valueNoise(x * freq, y * freq) * 2.0f - 1.0f; // [-1,1]
            sum += n * amp;
            norm += amp;
            amp *= persistence;
            freq *= lacunarity;
        }
        return sum / norm; // normalized back to roughly [-1,1]
    }
}
// -----------------------------------------------------------------------------------------

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


float Volcano::gaussian(float x, float z) {

    float exponent = -(x * x + z * z) / (2.0f * GAUSSIAN_SIGMA_SQ);
    return m_heightScale * std::exp(exponent);
}

void Volcano::applyRiverPath() {
    //leave empty for now

}

void Volcano::generateGeometry() {
    std::vector<glm::vec3> gridPositions;
    std::vector<glm::vec2> gridTexCoords;

    float step = m_width / (m_gridSize - 1);

    // Noise parameters
    constexpr float NOISE_FREQ = 0.15f;          // base frequency (lower -> broader features)
    constexpr float NOISE_AMPLITUDE = 0.35f;     // scales added noise relative to heightScale
    constexpr int   NOISE_OCTAVES = 5;
    constexpr float NOISE_PERSISTENCE = 0.5f;
    constexpr float NOISE_LACUNARITY = 2.1f;

    for (int i=0; i<m_gridSize; ++i) {
        for (int j=0; j<m_gridSize; ++j) {
            float x = (float)j * step - (m_width / 2.0f);
            float z = (float)i * step - (m_width / 2.0f);
            float base = gaussian(x, z);
            // Fractal noise sample (translate coordinates to avoid symmetry artifacts)
            float n = fractalNoise((x + 123.456f) * NOISE_FREQ, (z - 78.321f) * NOISE_FREQ, NOISE_OCTAVES, NOISE_PERSISTENCE, NOISE_LACUNARITY);
            // Modulate: add both additive and multiplicative variation
            float y = base + NOISE_AMPLITUDE * m_heightScale * n + base * 0.25f * n;
            if (y < 0.0f) y = 0.0f; // clamp
            gridPositions.push_back(glm::vec3(x, y, z));
            gridTexCoords.push_back(glm::vec2((float)j / (m_gridSize - 1), (float)i / (m_gridSize - 1)));
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
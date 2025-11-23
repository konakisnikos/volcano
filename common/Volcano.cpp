#include "Volcano.h"
#include <cmath>
#include <iostream>

namespace {
// Classic Perlin permutation table
constexpr int kPermutation[256] = {
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,
    168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,
    111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208,
    89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,
    186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,
    82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,
    183,170,213,119,248,152, 2,44,154,163,70,221,153,101,155,167,
    43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,
    185, 112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,
    179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,
    199,106,157,184,84,204,176,115,121,50,45,127, 4,150,254,138,
    236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215
};

constexpr int kPermSize = 512;

int kPerm[kPermSize];

struct PermInitializer {
    PermInitializer() {
        for (int i = 0; i < kPermSize; ++i) {
            kPerm[i] = kPermutation[i & 255];
        }
    }
};

PermInitializer g_permInitializer;

inline float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float grad(int hash, float x, float y) {
    int h = hash & 7; // 8 gradients
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float perlin2D(float x, float y) {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = kPerm[X + kPerm[Y]];
    int ab = kPerm[X + kPerm[Y + 1]];
    int ba = kPerm[X + 1 + kPerm[Y]];
    int bb = kPerm[X + 1 + kPerm[Y + 1]];

    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0f, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);

    return lerp(x1, x2, v);
}

float fractalPerlin(float x, float y, int octaves, float persistence, float lacunarity) {
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float value = 0.0f;
    float maxAmplitude = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += perlin2D(x * frequency, y * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxAmplitude; // normalize to roughly [-1,1]
}
} // namespace

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

    constexpr float BASE_FREQUENCY = 0.05f;
    constexpr int NOISE_OCTAVES = 5;
    constexpr float NOISE_PERSISTENCE = 0.5f;
    constexpr float NOISE_LACUNARITY = 2.0f;
    const glm::vec2 NOISE_OFFSET(113.7f, -57.3f);

    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {
            float x = static_cast<float>(j) * step - halfWidth;
            float z = static_cast<float>(i) * step - halfWidth;

            float sample = fractalPerlin(
                (x + NOISE_OFFSET.x) * BASE_FREQUENCY,
                (z + NOISE_OFFSET.y) * BASE_FREQUENCY,
                NOISE_OCTAVES,
                NOISE_PERSISTENCE,
                NOISE_LACUNARITY
            );

            float y = sample * m_heightScale;

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
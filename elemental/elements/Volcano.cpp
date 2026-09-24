#include "Volcano.h"
#include "VolcanoNoise.h"
#include <cmath>
#include <iostream>
#include <SOIL.h>

Volcano::Volcano(int gridSize, float maxTerrainWidth, float heightScale, glm::vec2 volcanoCenter)
    : m_gridSize(gridSize), m_width(maxTerrainWidth), m_heightScale(heightScale), m_volcanoCenter(volcanoCenter), m_drawable(nullptr)
{
    loadHeightMap(ELEMENTAL_ASSET_DIR "/heightmap_8bit.png");
    generateGeometry();
    m_drawable = new Drawable(m_positions, m_texCoords, m_normals, false);
    // Keep custom attributes aligned with the generated vertices.
    m_drawable->addExtraAttribute(3, 1, m_riverMasks);
    m_drawable->addExtraAttribute(4, 1, m_distFromCenter);
    m_drawable->addExtraAttribute(5, 1, m_grassMasks);
    m_drawable->addExtraAttribute(6, 1, m_riverBankDistances);

    std::cout << "Data sent to GPU successfully." << std::endl;
    std::cout << "Positions Size: " << m_positions.size() << std::endl;
    std::cout << "RiverMasks Size: " << m_riverMasks.size() << std::endl;
    std::cout << "Volcano created with "
              << m_positions.size() << " vertices ("
              << m_positions.size() / 3 << " triangles)." << std::endl;
    std::cout << "crater radius: " << m_stats.craterRadius << " \r";
}

Volcano::~Volcano() {
    if (m_drawable) {
        delete m_drawable;
    }
}

float Volcano::surfaceHeightAt(float worldX, float worldZ) const {
    if (m_surfaceHeights.empty() || m_gridSize < 2) return 0.0f;

    const float halfWidth = m_width * 0.5f;
    const float gridX = glm::clamp((worldX + halfWidth) / m_width, 0.0f, 1.0f)
                      * static_cast<float>(m_gridSize - 1);
    const float gridZ = glm::clamp((worldZ + halfWidth) / m_width, 0.0f, 1.0f)
                      * static_cast<float>(m_gridSize - 1);
    const int x0 = static_cast<int>(std::floor(gridX));
    const int z0 = static_cast<int>(std::floor(gridZ));
    const int x1 = glm::min(x0 + 1, m_gridSize - 1);
    const int z1 = glm::min(z0 + 1, m_gridSize - 1);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);

    const float h00 = m_surfaceHeights[z0 * m_gridSize + x0];
    const float h10 = m_surfaceHeights[z0 * m_gridSize + x1];
    const float h01 = m_surfaceHeights[z1 * m_gridSize + x0];
    const float h11 = m_surfaceHeights[z1 * m_gridSize + x1];
    return glm::mix(glm::mix(h00, h10, tx), glm::mix(h01, h11, tx), tz);
}



void Volcano::generateGeometry() {
    // Generate one set of attributes per grid point before forming triangles.
    std::vector<glm::vec3> tempGridPositions;
    std::vector<glm::vec3> tempGridNormals;
    std::vector<glm::vec2> tempGridTexCoords;
    
    // Temporary vectors hold per-grid-point attributes before unrolling.
    std::vector<float> tempGridRiverMasks; 
    std::vector<float> tempGridRiverBankDistances;
    std::vector<float> tempGridGrassMasks;
    std::vector<float> tempGridDist;      

    // Reserve memory to prevent re-allocations
    tempGridPositions.reserve(m_gridSize * m_gridSize);
    tempGridNormals.resize(m_gridSize * m_gridSize, glm::vec3(0.0f, 1.0f, 0.0f));
    tempGridTexCoords.reserve(m_gridSize * m_gridSize);
    tempGridRiverMasks.reserve(m_gridSize * m_gridSize);
    tempGridRiverBankDistances.reserve(m_gridSize * m_gridSize);
    tempGridGrassMasks.reserve(m_gridSize * m_gridSize);
    tempGridDist.reserve(m_gridSize * m_gridSize);

    float step = m_width / (m_gridSize - 1);
    float halfWidth = m_width * 0.5f;

    // Noise and terrain constants.
    constexpr float BASE_FREQUENCY = 0.04f;   
    constexpr int NOISE_OCTAVES = 6;           
    constexpr float NOISE_PERSISTENCE = 0.65f;
    constexpr float NOISE_LACUNARITY = 1.8f;
    const glm::vec2 NOISE_OFFSET(113.7f, -57.3f);

    const float noiseAmplitude = m_heightScale * 0.15f;   
    const float volcanoSigma = m_width * 0.10f;
    const float volcanoAmplitude = m_heightScale * 11.4f;  

    const float craterRadius = volcanoSigma * 0.3f;       
    const float craterDepth  = volcanoAmplitude * 0.85f;  
    const float craterBlend  = craterRadius * 0.75f;      

    // River controls
    const float riverNoiseX = halfWidth * 0.03f;
    const float riverWidth = m_width * 0.010f;
    const float riverBlend = riverWidth * 2.2f;
    const float riverDepth = m_heightScale * 1.65f;       
    const float riverInnerRadius = craterRadius * 1.05f;  
    const float riverOuterRadius = riverInnerRadius + m_width * 0.55f;
    const float riverRadialBlend = volcanoSigma * 0.35f;  
    const float riverStartTargetX = 0.0f;                 
    const float riverStartRadius = craterRadius * 1.05f;  
    const float riverStartBlendRadius = riverStartRadius * 0.6f;
    
    // Update Stats
    m_stats.peakY = volcanoAmplitude; 
    m_stats.baseY = volcanoAmplitude * 0.05f; 
    m_stats.baseRadius = volcanoSigma * 3.0f; 
    m_stats.riverBedY = -riverDepth; 
    m_stats.riverWaterY = -riverDepth + (riverDepth * 0.8f); 
    m_stats.centerZ = m_volcanoCenter.y;
    
    // Crater-specific stats for lava plane
    m_stats.craterRadius = craterRadius;
    m_stats.craterBottom = volcanoAmplitude - craterDepth; // Peak minus depth
    m_stats.craterTop = volcanoAmplitude;                   // The rim is at peak
    m_stats.craterCenter = m_volcanoCenter;                 // (X, Z) position

    // The channel runs along +Z from the crater.
    m_stats.riverInnerRadius = riverInnerRadius;
    m_stats.riverOuterRadius = riverOuterRadius;
    m_stats.riverEndXZ = m_volcanoCenter + glm::vec2(riverStartTargetX, riverOuterRadius);
    m_stats.riverEndY = m_stats.riverBedY;
    float closestRiverEndDistance2 = 1.0e30f;

    auto sampleHeightMap = [this](int i, int j, float fallbackNoiseHeight) {
        if (m_heightMap.empty()) return fallbackNoiseHeight; 
        int idx = i * m_gridSize + j;
        if (idx < 0 || idx >= static_cast<int>(m_heightMap.size())) return fallbackNoiseHeight;
        return m_heightMap[idx] * (m_heightScale * 17.0f); 
    };

    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {
            float x = static_cast<float>(j) * step - halfWidth;
            float z = static_cast<float>(i) * step - halfWidth;

            float sample = volcano_noise::fractalPerlin(
                (x + NOISE_OFFSET.x) * BASE_FREQUENCY,
                (z + NOISE_OFFSET.y) * BASE_FREQUENCY,
                NOISE_OCTAVES,
                NOISE_PERSISTENCE,
                NOISE_LACUNARITY
            );

            float noiseHeight = sample * noiseAmplitude;
            noiseHeight = sampleHeightMap(i, j, noiseHeight);

            float xFromVolcano = x - m_volcanoCenter.x;
            float zFromVolcano = z - m_volcanoCenter.y;
            float distanceFromCenter = std::sqrt(xFromVolcano * xFromVolcano + zFromVolcano * zFromVolcano);

            // A pure Gaussian produces an unnaturally perfect cone. Warping the
            // radial distance makes a readable but still inexpensive volcanic
            // silhouette, while the higher-frequency term suggests erosion ridges.
            float angle = std::atan2(zFromVolcano, xFromVolcano);
            float silhouetteWarp = 1.0f
                                 + 0.10f * std::sin(angle * 3.0f + 0.65f)
                                 + 0.045f * std::sin(angle * 7.0f - 0.9f);
            float shapedX = xFromVolcano / silhouetteWarp;
            float shapedZ = zFromVolcano / silhouetteWarp;
            float volcanoHeight = volcano_noise::gaussianPeak(
                shapedX, shapedZ, volcanoSigma, volcanoAmplitude);

            float ridgeBand = glm::smoothstep(craterRadius * 1.15f,
                                               volcanoSigma * 1.35f,
                                               distanceFromCenter)
                            * (1.0f - glm::smoothstep(volcanoSigma * 2.15f,
                                                      volcanoSigma * 3.1f,
                                                      distanceFromCenter));
            float ridges = std::sin(angle * 9.0f + distanceFromCenter * 0.032f)
                         + 0.45f * std::sin(angle * 17.0f - distanceFromCenter * 0.018f);
            volcanoHeight *= 1.0f + ridges * 0.045f * ridgeBand;

            float localCraterRadius = craterRadius *
                (1.0f + 0.09f * std::sin(angle * 5.0f + 0.4f));
            float craterMask = glm::smoothstep(localCraterRadius + craterBlend,
                                                localCraterRadius,
                                                distanceFromCenter);
            float craterHeight = -craterDepth * craterMask;
            float y = noiseHeight + volcanoHeight + craterHeight;

            // Bring the outermost terrain gently down before it reaches the mesh
            // boundary. The shader fog completes the fade, so no square cliff is
            // visible from the establishing shot.
            float edgeDistance = halfWidth - glm::max(std::abs(x), std::abs(z));
            float edgeFade = glm::smoothstep(0.0f, m_width * 0.11f, edgeDistance);
            const float outerGroundY = -8.0f;
            y = outerGroundY + (y - outerGroundY) * edgeFade;

            // River Logic
            float sNoise = std::sin(zFromVolcano * 0.02f + 1.37f);
            float rawRiverCenterX = riverNoiseX * sNoise;
            float startBlend = glm::smoothstep(riverStartRadius, riverStartRadius + riverStartBlendRadius, distanceFromCenter);
            float riverCenterX = glm::mix(riverStartTargetX + m_volcanoCenter.x, rawRiverCenterX + m_volcanoCenter.x, startBlend);

            float riverDistanceX = std::abs(x - riverCenterX);
            float riverBankDistance = riverDistanceX - riverWidth;
            float lateralMask = 1.0f - glm::smoothstep(riverWidth, riverWidth + riverBlend, riverDistanceX);
            float radialMask = glm::smoothstep(riverInnerRadius - riverRadialBlend, riverInnerRadius + riverRadialBlend, distanceFromCenter) *
                               (1.0f - glm::smoothstep(riverOuterRadius - riverRadialBlend, riverOuterRadius + riverRadialBlend, distanceFromCenter));
            float forwardMask = glm::smoothstep(-1.0f, 0.00f, zFromVolcano);

            float riverMask = lateralMask * radialMask * forwardMask;

            // Grass begins around the volcano base instead of climbing toward
            // the crater. Downstream it opens into an intentionally broad damp
            // plain that reaches almost all the way across the terrain.
            const float grassStartRadius = m_stats.baseRadius * 0.76f;
            float riverProgress = glm::clamp(
                (distanceFromCenter - grassStartRadius)
                / (riverOuterRadius - grassStartRadius), 0.0f, 1.0f);
            float grassHalfWidth = glm::mix(335.0f, 640.0f, riverProgress);
            float grassLateral = 1.0f - glm::smoothstep(
                grassHalfWidth, grassHalfWidth + 105.0f, riverDistanceX);
            float grassRadial = glm::smoothstep(grassStartRadius,
                                                 grassStartRadius + 110.0f,
                                                 distanceFromCenter)
                              * (1.0f - glm::smoothstep(riverOuterRadius + 240.0f,
                                                        riverOuterRadius + 400.0f,
                                                        distanceFromCenter));
            float grassMask = grassLateral * grassRadial * forwardMask;
            
            y -= riverMask * riverDepth;

            // Record the generated surface height at the river mouth. Particle
            // effects and vegetation use this instead of an approximate negative
            // trench depth, which could otherwise place them below the terrain.
            float endDx = x - m_stats.riverEndXZ.x;
            float endDz = z - m_stats.riverEndXZ.y;
            float endDistance2 = endDx * endDx + endDz * endDz;
            if (endDistance2 < closestRiverEndDistance2) {
                closestRiverEndDistance2 = endDistance2;
                m_stats.riverEndY = y;
            }

            // Store synchronized grid attributes before triangle unrolling.
            tempGridPositions.emplace_back(x, y, z);
            tempGridTexCoords.emplace_back(
                static_cast<float>(j) / (m_gridSize - 1),
                static_cast<float>(i) / (m_gridSize - 1)
            );
            tempGridRiverMasks.push_back(riverMask);       // Store cleanly
            tempGridRiverBankDistances.push_back(riverBankDistance);
            tempGridGrassMasks.push_back(grassMask);
            tempGridDist.push_back(distanceFromCenter);    // Store cleanly
        }
    }

    // Keep only the height component for inexpensive particle/terrain contact
    // queries. The render mesh remains exactly the same resolution.
    m_surfaceHeights.resize(tempGridPositions.size());
    for (std::size_t i = 0; i < tempGridPositions.size(); ++i) {
        m_surfaceHeights[i] = tempGridPositions[i].y;
    }

    // Central differences produce smooth per-vertex normals.
    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {
            int leftJ = glm::max(0, j - 1);
            int rightJ = glm::min(m_gridSize - 1, j + 1);
            int backI = glm::max(0, i - 1);
            int frontI = glm::min(m_gridSize - 1, i + 1);

            const glm::vec3 tangentX = tempGridPositions[i * m_gridSize + rightJ]
                                     - tempGridPositions[i * m_gridSize + leftJ];
            const glm::vec3 tangentZ = tempGridPositions[frontI * m_gridSize + j]
                                     - tempGridPositions[backI * m_gridSize + j];
            tempGridNormals[i * m_gridSize + j] =
                glm::normalize(glm::cross(tangentZ, tangentX));
        }
    }

    // Expand the grid into triangles while preserving attribute order.
    m_positions.clear();
    m_normals.clear();
    m_texCoords.clear();
    m_riverMasks.clear();
    m_riverBankDistances.clear();
    m_grassMasks.clear();
    m_distFromCenter.clear();

    // Two triangles, six vertices per grid cell.
    size_t numIndices = (m_gridSize - 1) * (m_gridSize - 1) * 6;
    m_positions.reserve(numIndices);
    m_normals.reserve(numIndices);
    m_texCoords.reserve(numIndices);
    m_riverMasks.reserve(numIndices);
    m_riverBankDistances.reserve(numIndices);
    m_grassMasks.reserve(numIndices);
    m_distFromCenter.reserve(numIndices);

    for (int i=0; i<m_gridSize - 1; ++i) {
        for (int j=0; j<m_gridSize - 1; ++j) {
            int v0 = i * m_gridSize + j;       // Top-Left
            int v1 = (i + 1) * m_gridSize + j; // Bottom-Left
            int v2 = v0 + 1;                   // Top-Right
            int v3 = v1 + 1;                   // Bottom-Right

            // Append position and custom attributes in the same order.
            auto pushVertex = [&](int index) {
                m_positions.push_back(tempGridPositions[index]);
                m_normals.push_back(tempGridNormals[index]);
                m_texCoords.push_back(tempGridTexCoords[index]);
                m_riverMasks.push_back(tempGridRiverMasks[index]); // Match mask to pos
                m_riverBankDistances.push_back(tempGridRiverBankDistances[index]);
                m_grassMasks.push_back(tempGridGrassMasks[index]);
                m_distFromCenter.push_back(tempGridDist[index]);   // Match dist to pos
            };

            // Triangle 1: V0 -> V1 -> V3
            pushVertex(v0);
            pushVertex(v1);
            pushVertex(v3);

            // Triangle 2: V0 -> V3 -> V2
            pushVertex(v0);
            pushVertex(v3);
            pushVertex(v2);
        }
    }

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

void Volcano::loadHeightMap(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
    if (!data) {
        std::cerr << "Failed to load heightmap image: " << path << "\n";
        return;
    }

    if (width <= 0 || height <= 0) {
        std::cerr << "Invalid heightmap dimensions: " << width << "x" << height << " for " << path << "\n";
        SOIL_free_image_data(data);
        return;
    }

    m_heightMap.clear();
    m_heightMap.resize(m_gridSize * m_gridSize, 0.0f);

    // 1. Read the raw 8-bit data
    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {
            float u = static_cast<float>(j) / (m_gridSize - 1);
            float v = static_cast<float>(i) / (m_gridSize - 1);

            int imgX = static_cast<int>(u * (width  - 1));
            int imgY = static_cast<int>(v * (height - 1));

            int idx = (imgY * width + imgX) * channels;
            unsigned char r = data[idx];
            unsigned char g = channels > 1 ? data[idx + 1] : r;
            unsigned char b = channels > 2 ? data[idx + 2] : r;

            float gray = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / (3.0f * 255.0f);
            m_heightMap[i * m_gridSize + j] = gray;
        }
    }

    SOIL_free_image_data(data);

    // Repeated smoothing softens quantization in the 8-bit heightmap.
    for (int k = 0; k < 3; ++k) {
        volcano_noise::smoothHeightMapData(m_heightMap, m_gridSize);
    }
}

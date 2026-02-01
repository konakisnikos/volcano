#include "Volcano.h"
#include "VolcanoNoise.h"
#include <cmath>
#include <iostream>
#include <SOIL.h>

Volcano::Volcano(int gridSize, float maxTerrainWidth, float heightScale, glm::vec2 volcanoCenter)
    : m_gridSize(gridSize), m_width(maxTerrainWidth), m_heightScale(heightScale), m_volcanoCenter(volcanoCenter), m_drawable(nullptr)
{
    loadHeightMap("/Users/nikos/Desktop/elemental/elemental/assets/heightmap_8bit.png");
    generateGeometry();
    calculateNormals();
    m_drawable = new Drawable(m_positions, m_texCoords, m_normals, false);
    // Send River Mask to Layout 3
    m_drawable->addExtraAttribute(3, 1, m_riverMasks);
    
    // Send Distance to Layout 4
    m_drawable->addExtraAttribute(4, 1, m_distFromCenter);

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



void Volcano::generateGeometry() {
    // ------------------------------------------------------------------
    // STEP 1: Generate the Grid Data (The "Source")
    // ------------------------------------------------------------------
    std::vector<glm::vec3> tempGridPositions;
    std::vector<glm::vec2> tempGridTexCoords;
    
    // NEW: Temporary vectors to store the math results before unrolling
    std::vector<float> tempGridRiverMasks; 
    std::vector<float> tempGridDist;      

    // Reserve memory to prevent re-allocations
    tempGridPositions.reserve(m_gridSize * m_gridSize);
    tempGridTexCoords.reserve(m_gridSize * m_gridSize);
    tempGridRiverMasks.reserve(m_gridSize * m_gridSize);
    tempGridDist.reserve(m_gridSize * m_gridSize);

    float step = m_width / (m_gridSize - 1);
    float halfWidth = m_width * 0.5f;

    // --- Noise & Terrain Constants (Kept exactly as you had them) ---
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

    // Helper Lambda
    auto sampleHeightMap = [this](int i, int j, float fallbackNoiseHeight) {
        if (m_heightMap.empty()) return fallbackNoiseHeight; 
        int idx = i * m_gridSize + j;
        if (idx < 0 || idx >= static_cast<int>(m_heightMap.size())) return fallbackNoiseHeight;
        return m_heightMap[idx] * (m_heightScale * 17.0f); 
    };

    // --- LOOP 1: Calculate Math for every Grid Point ---
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
            float volcanoHeight = volcano_noise::gaussianPeak(xFromVolcano, zFromVolcano, volcanoSigma, volcanoAmplitude);
            float distanceFromCenter = std::sqrt(xFromVolcano * xFromVolcano + zFromVolcano * zFromVolcano);

            float craterMask = glm::smoothstep(craterRadius + craterBlend, craterRadius, distanceFromCenter);
            float craterHeight = -craterDepth * craterMask;
            float y = noiseHeight + volcanoHeight + craterHeight;

            // River Logic
            float sNoise = std::sin(zFromVolcano * 0.02f + 1.37f);
            float rawRiverCenterX = riverNoiseX * sNoise;
            float startBlend = glm::smoothstep(riverStartRadius, riverStartRadius + riverStartBlendRadius, distanceFromCenter);
            float riverCenterX = glm::mix(riverStartTargetX + m_volcanoCenter.x, rawRiverCenterX + m_volcanoCenter.x, startBlend);

            float riverDistanceX = std::abs(x - riverCenterX);
            float lateralMask = 1.0f - glm::smoothstep(riverWidth, riverWidth + riverBlend, riverDistanceX);
            float radialMask = glm::smoothstep(riverInnerRadius - riverRadialBlend, riverInnerRadius + riverRadialBlend, distanceFromCenter) *
                               (1.0f - glm::smoothstep(riverOuterRadius - riverRadialBlend, riverOuterRadius + riverRadialBlend, distanceFromCenter));
            float forwardMask = glm::smoothstep(-1.0f, 0.00f, zFromVolcano);

            float riverMask = lateralMask * radialMask * forwardMask;
            
            y -= riverMask * riverDepth;

            // --- CRITICAL FIX: Store to TEMP vectors first ---
            tempGridPositions.emplace_back(x, y, z);
            tempGridTexCoords.emplace_back(
                static_cast<float>(j) / (m_gridSize - 1),
                static_cast<float>(i) / (m_gridSize - 1)
            );
            tempGridRiverMasks.push_back(riverMask);       // Store cleanly
            tempGridDist.push_back(distanceFromCenter);    // Store cleanly
        }
    }

    // ------------------------------------------------------------------
    // STEP 2: The "Unrolling" (Syncing Grid Data to Triangles)
    // ------------------------------------------------------------------
    
    // Clear the final member vectors so we start fresh
    m_positions.clear();
    m_texCoords.clear();
    m_riverMasks.clear();     // <--- Crucial clean-up
    m_distFromCenter.clear(); // <--- Crucial clean-up

    // We can reserve size to speed it up (6 vertices per grid square)
    size_t numIndices = (m_gridSize - 1) * (m_gridSize - 1) * 6;
    m_positions.reserve(numIndices);
    m_texCoords.reserve(numIndices);
    m_riverMasks.reserve(numIndices);
    m_distFromCenter.reserve(numIndices);

    for (int i=0; i<m_gridSize - 1; ++i) {
        for (int j=0; j<m_gridSize - 1; ++j) {
            // Get the indices for the 4 corners in the GRID list
            int v0 = i * m_gridSize + j;       // Top-Left
            int v1 = (i + 1) * m_gridSize + j; // Bottom-Left
            int v2 = v0 + 1;                   // Top-Right
            int v3 = v1 + 1;                   // Bottom-Right

            // Helper lambda to push ONE vertex and ALL its attributes
            // This guarantees they are always perfectly synced.
            auto pushVertex = [&](int index) {
                m_positions.push_back(tempGridPositions[index]);
                m_texCoords.push_back(tempGridTexCoords[index]);
                m_riverMasks.push_back(tempGridRiverMasks[index]); // Match mask to pos
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

    // Done! All member vectors are now populated and perfectly synchronized.
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

    // ---------------------------------------------------------
    // 2. THE PIXAR SMOOTHING STEP (Fixes the Staircase effect)
    // ---------------------------------------------------------
    // Run the smoother multiple times. 
    // 10 times is usually enough to turn "stairs" into a "slope".
    for (int k = 0; k < 3; ++k) {
        volcano_noise::smoothHeightMapData(m_heightMap, m_gridSize);
    }
    // ---------------------------------------------------------
}

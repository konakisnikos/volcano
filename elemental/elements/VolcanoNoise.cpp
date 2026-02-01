#include "VolcanoNoise.h"

#include <cmath>

namespace volcano_noise {
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
} // namespace

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

float gaussianPeak(float x, float z, float sigma, float amplitude) {
    float sigmaSq = sigma * sigma;
    float distSq = x * x + z * z;
    return amplitude * std::exp(-distSq / (2.0f * sigmaSq));
}

void smoothHeightMapData(std::vector<float>& heights, int gridSize) {
    std::vector<float> smoothed = heights; // Create a copy

    for (int i = 1; i < gridSize - 1; ++i) {
        for (int j = 1; j < gridSize - 1; ++j) {
            float total = 0.0f;

            // Average 3x3 neighborhood
            total += heights[(i - 1) * gridSize + (j - 1)]; // Top-Left
            total += heights[(i - 1) * gridSize + j];       // Top
            total += heights[(i - 1) * gridSize + (j + 1)]; // Top-Right

            total += heights[i * gridSize + (j - 1)];       // Left
            total += heights[i * gridSize + j];             // Center
            total += heights[i * gridSize + (j + 1)];       // Right

            total += heights[(i + 1) * gridSize + (j - 1)]; // Bottom-Left
            total += heights[(i + 1) * gridSize + j];       // Bottom
            total += heights[(i + 1) * gridSize + (j + 1)]; // Bottom-Right

            smoothed[i * gridSize + j] = total / 9.0f;
        }
    }
    heights = smoothed; // Apply changes
}

} // namespace volcano_noise

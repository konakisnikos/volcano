#ifndef VOLCANO_NOISE_H
#define VOLCANO_NOISE_H

#include <vector>

namespace volcano_noise {

float perlin2D(float x, float y);
float fractalPerlin(float x, float y, int octaves, float persistence, float lacunarity);
float gaussianPeak(float x, float z, float sigma, float amplitude);
void smoothHeightMapData(std::vector<float>& heights, int gridSize);

} // namespace volcano_noise

#endif // VOLCANO_NOISE_H

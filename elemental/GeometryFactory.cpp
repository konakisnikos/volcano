#include "GeometryFactory.h"

#include <cmath>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <common/model.h>
#include <elemental/elements/Volcano.h>

namespace {
glm::vec3 spherePoint(float theta, float phi) {
    const float ring = std::sin(theta);
    return glm::vec3(ring * std::cos(phi), std::cos(theta), ring * std::sin(phi));
}
}

Drawable* createUnitSphereDrawable(int stacks, int slices) {
    const float pi = 3.14159265359f;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    vertices.reserve(stacks * slices * 6);
    normals.reserve(stacks * slices * 6);
    uvs.reserve(stacks * slices * 6);

    auto append = [&](const glm::vec3& position, float u, float v) {
        vertices.push_back(position);
        normals.push_back(glm::normalize(position));
        uvs.push_back(glm::vec2(u, v));
    };

    for (int stack = 0; stack < stacks; ++stack) {
        const float v0 = static_cast<float>(stack) / stacks;
        const float v1 = static_cast<float>(stack + 1) / stacks;
        const float theta0 = v0 * pi;
        const float theta1 = v1 * pi;
        for (int slice = 0; slice < slices; ++slice) {
            const float u0 = static_cast<float>(slice) / slices;
            const float u1 = static_cast<float>(slice + 1) / slices;
            const float phi0 = u0 * 2.0f * pi;
            const float phi1 = u1 * 2.0f * pi;

            const glm::vec3 a = spherePoint(theta0, phi0);
            const glm::vec3 b = spherePoint(theta1, phi0);
            const glm::vec3 c = spherePoint(theta1, phi1);
            const glm::vec3 d = spherePoint(theta0, phi1);

            // Counter-clockwise winding as seen from outside the sphere.
            append(a, u0, v0); append(c, u1, v1); append(b, u0, v1);
            append(a, u0, v0); append(d, u1, v0); append(c, u1, v1);
        }
    }

    return new Drawable(vertices, uvs, normals, false);
}

Drawable* createUnitCylinderDrawable(int sides) {
    const float pi = 3.14159265359f;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    vertices.reserve(sides * 12);
    normals.reserve(sides * 12);
    uvs.reserve(sides * 12);

    auto append = [&](const glm::vec3& position,
                      const glm::vec3& normal,
                      const glm::vec2& uv) {
        vertices.push_back(position);
        normals.push_back(normal);
        uvs.push_back(uv);
    };

    for (int side = 0; side < sides; ++side) {
        const float u0 = static_cast<float>(side) / sides;
        const float u1 = static_cast<float>(side + 1) / sides;
        const float a0 = u0 * 2.0f * pi;
        const float a1 = u1 * 2.0f * pi;
        const glm::vec3 n0(std::cos(a0), 0.0f, std::sin(a0));
        const glm::vec3 n1(std::cos(a1), 0.0f, std::sin(a1));
        const glm::vec3 b0(n0.x, -1.0f, n0.z);
        const glm::vec3 b1(n1.x, -1.0f, n1.z);
        const glm::vec3 t0(n0.x, 1.0f, n0.z);
        const glm::vec3 t1(n1.x, 1.0f, n1.z);

        append(b0, n0, glm::vec2(u0, 0.0f));
        append(t1, n1, glm::vec2(u1, 1.0f));
        append(t0, n0, glm::vec2(u0, 1.0f));
        append(b0, n0, glm::vec2(u0, 0.0f));
        append(b1, n1, glm::vec2(u1, 0.0f));
        append(t1, n1, glm::vec2(u1, 1.0f));

        // Flat caps keep stems and trunks closed when viewed from above/below.
        append(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f));
        append(t0, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f));
        append(t1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f));

        append(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f));
        append(b1, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f));
        append(b0, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f));
    }

    return new Drawable(vertices, uvs, normals, false);
}

Drawable* createGrassFieldDrawable(const Volcano& terrain) {
    const VolcanoStats stats = terrain.getStats();
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<float> baseHeights;
    std::vector<float> growthThresholds;

    constexpr int TUFT_COUNT = 1900;
    constexpr int BLADES_PER_TUFT = 4;
    constexpr float TERRAIN_HALF_WIDTH = 675.0f;
    vertices.reserve(TUFT_COUNT * BLADES_PER_TUFT * 9);
    normals.reserve(vertices.capacity());
    uvs.reserve(vertices.capacity());
    baseHeights.reserve(vertices.capacity());
    growthThresholds.reserve(vertices.capacity());

    // A local seeded generator keeps the same grass arrangement on every run
    // without changing the random sequence used by smoke, lightning or flowers.
    std::mt19937 generator(4242u);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    auto randomRange = [&](float minimum, float maximum) {
        return glm::mix(minimum, maximum, random01(generator));
    };

    auto appendVertex = [&](const glm::vec3& position,
                            const glm::vec3& normal,
                            const glm::vec2& uv,
                            float baseY,
                            float threshold) {
        vertices.push_back(position);
        normals.push_back(normal);
        uvs.push_back(uv);
        baseHeights.push_back(baseY);
        growthThresholds.push_back(threshold);
    };

    const float grassStartRadius = stats.baseRadius * 0.76f;
    int acceptedTufts = 0;
    int attempts = 0;
    while (acceptedTufts < TUFT_COUNT && attempts < TUFT_COUNT * 30) {
        ++attempts;
        const float x = randomRange(-TERRAIN_HALF_WIDTH + 28.0f,
                                     TERRAIN_HALF_WIDTH - 28.0f);
        const float z = randomRange(-112.0f, TERRAIN_HALF_WIDTH - 28.0f);
        const float xFromVolcano = x - stats.craterCenter.x;
        const float zFromVolcano = z - stats.craterCenter.y;
        const float distance = std::sqrt(xFromVolcano * xFromVolcano
                                       + zFromVolcano * zFromVolcano);

        // Match the terrain's curved river and broad lower-plain vegetation mask.
        const float rawCenterX = TERRAIN_HALF_WIDTH * 0.03f
                               * std::sin(zFromVolcano * 0.02f + 1.37f);
        const float startRadius = stats.craterRadius * 1.05f;
        const float startBlend = glm::smoothstep(startRadius,
                                                  startRadius + startRadius * 0.6f,
                                                  distance);
        const float riverCenterX = glm::mix(stats.craterCenter.x,
                                             stats.craterCenter.x + rawCenterX,
                                             startBlend);
        const float distanceFromRiver = std::abs(x - riverCenterX);
        const float riverProgress = glm::clamp(
            (distance - grassStartRadius)
            / (stats.riverOuterRadius - grassStartRadius), 0.0f, 1.0f);
        const float halfWidth = glm::mix(335.0f, 640.0f, riverProgress);
        const float lateralMask = 1.0f - glm::smoothstep(
            halfWidth, halfWidth + 105.0f, distanceFromRiver);
        const float radialMask = glm::smoothstep(grassStartRadius,
                                                  grassStartRadius + 110.0f,
                                                  distance)
                               * (1.0f - glm::smoothstep(stats.riverOuterRadius + 240.0f,
                                                         stats.riverOuterRadius + 400.0f,
                                                         distance));
        const float fieldMask = lateralMask * radialMask
                              * glm::smoothstep(-1.0f, 0.0f, zFromVolcano);

        // Preserve a clean water edge and use the soft mask as density rather
        // than producing a visibly straight boundary around the field.
        if (distanceFromRiver < 38.0f || fieldMask < 0.12f
            || random01(generator) > fieldMask) {
            continue;
        }

        const float sampleStep = 4.0f;
        const float groundY = terrain.surfaceHeightAt(x, z) + 0.45f;
        const float heightLeft = terrain.surfaceHeightAt(x - sampleStep, z);
        const float heightRight = terrain.surfaceHeightAt(x + sampleStep, z);
        const float heightBack = terrain.surfaceHeightAt(x, z - sampleStep);
        const float heightFront = terrain.surfaceHeightAt(x, z + sampleStep);
        const glm::vec3 terrainNormal = glm::normalize(glm::vec3(
            heightLeft - heightRight, 2.0f * sampleStep,
            heightBack - heightFront));
        if (terrainNormal.y < 0.76f) continue;

        // Once the river is full, moisture spreads outward: close tufts sprout
        // first and the more distant tufts follow later.
        const float lateralProgress = glm::clamp(
            (distanceFromRiver - 38.0f) / glm::max(halfWidth - 38.0f, 1.0f),
            0.0f, 1.0f);
        const float threshold = glm::clamp(
            0.03f + lateralProgress * 0.58f + random01(generator) * 0.15f,
            0.02f, 0.80f);

        for (int blade = 0; blade < BLADES_PER_TUFT; ++blade) {
            const float angle = randomRange(0.0f, 6.2831853f);
            const glm::vec2 facing(std::cos(angle), std::sin(angle));
            const glm::vec2 sideways(-facing.y, facing.x);
            const float tuftRadius = randomRange(0.0f, 3.2f);
            const float tuftAngle = randomRange(0.0f, 6.2831853f);
            const glm::vec2 offset(std::cos(tuftAngle) * tuftRadius,
                                   std::sin(tuftAngle) * tuftRadius);
            const float bladeHeight = randomRange(4.5f, 8.5f);
            const float bladeWidth = randomRange(0.75f, 1.45f);
            const glm::vec2 lean = facing * randomRange(0.5f, 1.4f);

            const glm::vec3 baseCenter(x + offset.x, groundY, z + offset.y);
            const glm::vec3 lowerLeft = baseCenter
                - glm::vec3(sideways.x, 0.0f, sideways.y) * (bladeWidth * 0.5f);
            const glm::vec3 lowerRight = baseCenter
                + glm::vec3(sideways.x, 0.0f, sideways.y) * (bladeWidth * 0.5f);
            const glm::vec3 upperCenter = baseCenter
                + glm::vec3(lean.x * 0.68f, bladeHeight * 0.68f, lean.y * 0.68f);
            const glm::vec3 upperLeft = upperCenter
                - glm::vec3(sideways.x, 0.0f, sideways.y) * (bladeWidth * 0.24f);
            const glm::vec3 upperRight = upperCenter
                + glm::vec3(sideways.x, 0.0f, sideways.y) * (bladeWidth * 0.24f);
            const glm::vec3 tip = baseCenter
                + glm::vec3(lean.x, bladeHeight, lean.y);
            glm::vec3 bladeNormal = glm::normalize(glm::cross(lowerRight - lowerLeft,
                                                               tip - lowerLeft));
            if (bladeNormal.y < 0.0f) bladeNormal = -bladeNormal;

            appendVertex(lowerLeft,  bladeNormal, glm::vec2(0.0f, 0.0f), groundY, threshold);
            appendVertex(lowerRight, bladeNormal, glm::vec2(1.0f, 0.0f), groundY, threshold);
            appendVertex(upperRight, bladeNormal, glm::vec2(0.76f, 0.68f), groundY, threshold);
            appendVertex(lowerLeft,  bladeNormal, glm::vec2(0.0f, 0.0f), groundY, threshold);
            appendVertex(upperRight, bladeNormal, glm::vec2(0.76f, 0.68f), groundY, threshold);
            appendVertex(upperLeft,  bladeNormal, glm::vec2(0.24f, 0.68f), groundY, threshold);
            appendVertex(upperLeft,  bladeNormal, glm::vec2(0.24f, 0.68f), groundY, threshold);
            appendVertex(upperRight, bladeNormal, glm::vec2(0.76f, 0.68f), groundY, threshold);
            appendVertex(tip,        bladeNormal, glm::vec2(0.5f, 1.0f), groundY, threshold);
        }
        ++acceptedTufts;
    }

    Drawable* grass = new Drawable(vertices, uvs, normals, false);
    grass->addExtraAttribute(3, 1, baseHeights);
    grass->addExtraAttribute(4, 1, growthThresholds);
    return grass;
}

#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

struct ParticleShaderParams {
    float sizeScale = 1.0f;

    float wobbleAmp = 0.008f;
    glm::vec2 wobbleTimeFreq = glm::vec2(1.2f, 1.0f);
    float wobbleUVFreq = 7.0f;

    glm::vec2 featherEdges = glm::vec2(1.0f, 0.55f);
    glm::vec2 densityEdges = glm::vec2(0.01f, 0.75f);
    float alphaDiscard = 0.002f;

    glm::vec2 rimEdges = glm::vec2(0.25f, 0.95f);
    float rimIntensity = 0.12f;

    float texRgbMix = 0.55f;
    glm::vec2 densityMulAdd = glm::vec2(0.15f, 0.85f);
};

struct ParticleShaderUniformLocations {
    GLint projection = -1;
    GLint view = -1;
    GLint moonColor = -1;
    GLint time = -1;
    GLint smokeTexture = -1;

    GLint sizeScale = -1;

    GLint wobbleAmp = -1;
    GLint wobbleTimeFreq = -1;
    GLint wobbleUVFreq = -1;

    GLint featherEdges = -1;
    GLint densityEdges = -1;
    GLint alphaDiscard = -1;

    GLint rimEdges = -1;
    GLint rimIntensity = -1;

    GLint texRgbMix = -1;
    GLint densityMulAdd = -1;
};

inline ParticleShaderUniformLocations getParticleShaderUniformLocations(GLuint program) {
    ParticleShaderUniformLocations u;

    u.projection = glGetUniformLocation(program, "projection");
    u.view = glGetUniformLocation(program, "view");
    u.moonColor = glGetUniformLocation(program, "moonColor");
    u.time = glGetUniformLocation(program, "uTime");
    u.smokeTexture = glGetUniformLocation(program, "uSmokeTexture");

    u.sizeScale = glGetUniformLocation(program, "uSizeScale");

    u.wobbleAmp = glGetUniformLocation(program, "uWobbleAmp");
    u.wobbleTimeFreq = glGetUniformLocation(program, "uWobbleTimeFreq");
    u.wobbleUVFreq = glGetUniformLocation(program, "uWobbleUVFreq");

    u.featherEdges = glGetUniformLocation(program, "uFeatherEdges");
    u.densityEdges = glGetUniformLocation(program, "uDensityEdges");
    u.alphaDiscard = glGetUniformLocation(program, "uAlphaDiscard");

    u.rimEdges = glGetUniformLocation(program, "uRimEdges");
    u.rimIntensity = glGetUniformLocation(program, "uRimIntensity");

    u.texRgbMix = glGetUniformLocation(program, "uTexRgbMix");
    u.densityMulAdd = glGetUniformLocation(program, "uDensityMulAdd");

    return u;
}

inline void applyParticleShaderParams(
    const ParticleShaderUniformLocations& u,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& moonColor,
    float timeSeconds,
    const ParticleShaderParams& p
) {
    if (u.projection != -1) glUniformMatrix4fv(u.projection, 1, GL_FALSE, &projection[0][0]);
    if (u.view != -1) glUniformMatrix4fv(u.view, 1, GL_FALSE, &view[0][0]);
    if (u.moonColor != -1) glUniform3f(u.moonColor, moonColor.x, moonColor.y, moonColor.z);
    if (u.time != -1) glUniform1f(u.time, timeSeconds);

    if (u.sizeScale != -1) glUniform1f(u.sizeScale, p.sizeScale);

    if (u.wobbleAmp != -1) glUniform1f(u.wobbleAmp, p.wobbleAmp);
    if (u.wobbleTimeFreq != -1) glUniform2f(u.wobbleTimeFreq, p.wobbleTimeFreq.x, p.wobbleTimeFreq.y);
    if (u.wobbleUVFreq != -1) glUniform1f(u.wobbleUVFreq, p.wobbleUVFreq);

    if (u.featherEdges != -1) glUniform2f(u.featherEdges, p.featherEdges.x, p.featherEdges.y);
    if (u.densityEdges != -1) glUniform2f(u.densityEdges, p.densityEdges.x, p.densityEdges.y);
    if (u.alphaDiscard != -1) glUniform1f(u.alphaDiscard, p.alphaDiscard);

    if (u.rimEdges != -1) glUniform2f(u.rimEdges, p.rimEdges.x, p.rimEdges.y);
    if (u.rimIntensity != -1) glUniform1f(u.rimIntensity, p.rimIntensity);

    if (u.texRgbMix != -1) glUniform1f(u.texRgbMix, p.texRgbMix);
    if (u.densityMulAdd != -1) glUniform2f(u.densityMulAdd, p.densityMulAdd.x, p.densityMulAdd.y);
}

// Include C++ headers
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <stdio.h>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <random>
#include <sstream>
#include <SOIL.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/model.h>
#include <common/texture.h>
#include <elemental/elements/Volcano.h>
#include <elemental/elements/Skybox.h>
#include <elemental/SceneDirector.h>
#include <common/light.h>
#include <elemental/elements/SmokeEmitter.h>
#include <elemental/elements/CloudEmitter.h>
#include <elemental/elements/RainEmitter.h>
#include <elemental/LightningSystem.h>
#include <elemental/GeometryFactory.h>
#include <elemental/InstancedPropRenderer.h>

#if defined(ELEMENTAL_ENABLE_IMGUI)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

// Crack system (CPU -> shader + optional stone burst)
#include <elemental/CrackSystem.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
void resetSimulation(double realTimeSeconds);

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "ELEMENTAL"

bool gWindowed = false;
bool gAutoExitOnComplete = false;
bool gReportPerformance = false;
float gInitialTimeScale = 1.0f;
std::string gScreenshotPath;
bool gScreenshotCaptured = false;
SimulationStage gScreenshotStage = SimulationStage::Complete;
float gScreenshotStageDelay = 0.0f;
const unsigned int SIMULATION_RANDOM_SEED = 1337;
std::mt19937 gLightningRandom(SIMULATION_RANDOM_SEED + 1u);

static float lightningRandom01()
{
    static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(gLightningRandom);
}

// Global variables
GLFWwindow* window;
Camera* camera;
bool gOpenGLContextReady = false;
GLuint volcanoShaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint terrainTextureSampler;
GLuint terrainTexture;

struct VolcanoUniformLocations {
    GLint cameraPosition;
    GLint lightningFlash;
    GLint shadowMap;
    GLint lightViewProjection;
    GLint lavaTime;
    GLint craterRadius;
    GLint craterCenter;
    GLint scorchCount;
    GLint scorchPositions;
    GLint scorchRadii;
    GLint scorchStrengths;
    GLint lavaCoolStart;
    GLint lavaCoolDuration;
    GLint waterFill;
    GLint grassGrowth;
    GLint rainIntensity;
    GLint riverInnerRadius;
    GLint riverOuterRadius;
    GLint electricStart;
    GLint electricDuration;
    GLint electricStrikeDistance;
    GLint materialAmbient;
    GLint materialSpecular;
    GLint materialShininess;
};
VolcanoUniformLocations volcanoUniforms;
LightUniformLocations volcanoLightUniforms;

// Particle shader + smoke resources
GLuint particleShaderProgram;
GLuint particlePVLocation;
GLuint particleTextureSampler;
GLuint particleTintLocation;
GLuint particleAlphaLocation;
GLuint particleShapeScaleLocation;
GLuint particlePuffinessLocation;
GLuint particleShearLocation;
GLuint smokeTexture;
GLuint treeTexture;
GLuint almondTreeTexture;
Drawable* propSphere;     // procedural sphere used for flower props
Drawable* propCylinder;   // procedural cylinder used for flower stems
InstancedPropRenderer* propSphereRenderer;
InstancedPropRenderer* propCylinderRenderer;
Drawable* grassField;     // one combined mesh containing the grass blades
Drawable* particleQuad;   // flat camera-facing quad, used for all particle emitters
SmokeEmitter* smokeEmitter;
SmokeEmitter* calmSmokeEmitter = nullptr;
CloudEmitter* ambientCloudEmitter = nullptr;

// Domino chain (Part B.4a): ash/smoke emitted where the lava flow reaches the river end.
// Must match the "speed" constant in Volcano.vertexshader/fragmentshader so the CPU-side
// event trigger lines up with what's actually drawn.
const float LAVA_FLOW_SPEED = 20.0f;
const float LAVA_COOL_DURATION = 8.0f;
SmokeEmitter* ashEmitter = nullptr;
float lavaCoolStartTime = -1.0f; // lavaTime at which cooling began, or -1 if not yet

// Domino chain (Part B.4b): once ash has been rising for a while, it accumulates
// into a cloud hovering above the river end.
const float CLOUD_FORM_DELAY = 8.5f; // begins just after the slower cooling front completes
CloudEmitter* cloudEmitter = nullptr;

// Domino chain (Part B.4b/c): once the cloud has been forming for a while, it starts raining.
const float RAIN_START_DELAY = 3.0f; // seconds after the cloud appears before it starts raining
RainEmitter* rainEmitter = nullptr;
float rainStartTime = -1.0f; // lavaTime at which rain began, or -1 if not yet
const float WATER_FILL_DURATION = 22.0f; // slower rainfall/runoff accumulation
const float GRASS_GROW_DURATION = 6.0f; // starts only after the river reaches 100%
const float TREE_GROW_DELAY = 11.2f; // grass and flowers establish before trees
const float TREE_GROW_DURATION = 4.2f;

// Part B.6: cloud-to-ground lightning and persistent scorch marks.
LightningSystem* lightningSystem = nullptr;
std::vector<glm::vec2> scorchPositions;
std::vector<float> scorchRadii;
std::vector<float> scorchStrengths;
const float LIGHTNING_START_DELAY = 2.15f;
float nextLightningTime = -1.0f;
bool manualLightningRequested = false;
int earlyGroundStrikeCount = 0;
int vegetationGroundStrikeCount = 0;

// Part B.7: lightning + water. After three strikes make the grown meadow's
// scorch marks readable, one final bolt energises the river for a few seconds.
bool riverStrikeInFlight = false;
float electricRiverStartTime = -1.0f; // lava-time clock, also used by the shader
float electricRiverStrikeDistance = 0.0f;
const float ELECTRIC_RIVER_DURATION = 7.5f;
const float CALM_NIGHT_DURATION = 12.0f;

// Domino chain (Part B.4d): the river fills first, grass then grows across the
// damp terrain, and flowers appear last. All vegetation is procedural, so no
// external models are needed for this assignment stage.
struct PropInstance {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    float groundY;
    float growthDelay;
    float growthDuration;
    float swayPhase;
    float swayAmount;
    bool useCylinder;
};
GLuint propShaderProgram;
GLuint propMLoc, propVLoc, propPLoc, propColorLoc;
GLuint propGrassPassLoc, propInstancedPassLoc, propGrassGrowthLoc, propTimeLoc;
GLint propCameraLocation, propLightningLocation, propScorchCountLocation;
GLint propScorchPositionsLocation, propScorchRadiiLocation, propScorchStrengthsLocation;
LightUniformLocations propLightUniforms;
std::vector<PropInstance> floraProps;
std::vector<glm::mat4> spherePropMatrices;
std::vector<glm::vec3> spherePropColors;
std::vector<glm::mat4> cylinderPropMatrices;
std::vector<glm::vec3> cylinderPropColors;

struct TreeInstance {
    glm::vec3 basePosition;
    glm::vec2 size;
    glm::vec3 tint;
    float growthDelay;
    float swayPhase;
    bool flowering;
};
GLuint treeShaderProgram;
GLuint treeVLoc, treePLoc, treeBaseLoc, treeSizeLoc, treeGrowthLoc;
GLuint treeCameraLoc, treeTimeLoc, treeSwayLoc, treeTintLoc;
GLuint treeTextureLoc, treeLightningLoc;
std::vector<TreeInstance> treeProps;
bool floraSpawned = false;
float floraGrowthStartTime = -1.0f;

// Shadow mapping: depth-only pass rendered from the moonlight's POV
GLuint depthShaderProgram;
GLuint depthMLocation, depthVLocation, depthPLocation, depthLavaTimeLocation;
GLuint depthCraterRadiusLocation;
GLuint depthMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_MAP_SIZE = 2048;

#if defined(ELEMENTAL_ENABLE_IMGUI)
static bool gShowHud = false;
static bool gImGuiGlfwInitialized = false;
static bool gImGuiOpenGLInitialized = false;
#endif

Light* moonlight;

VolcanoStats stats;

struct Material {
    glm::vec4 Ks, Kd, Ka;
    GLfloat Ns;
};

Material volcanoMaterial = {
    glm::vec4(0.05f, 0.05f, 0.05f, 1.0f), // Ks
    glm::vec4(0.4f, 0.35f, 0.35f, 1.0f), // Kd
    glm::vec4(0.1f, 0.1f, 0.15f, 1.0f), // Ka
    10.0f                             // Ns
};

Volcano* volcano;
Skybox* skybox;
SceneDirector* sceneDirector;

// Position of the cooling boundary on the curved river centre line. The same
// outer-to-inner interpolation is used by the terrain shader, so the ash source
// and the visible black crust travel together.
static glm::vec3 coolingFrontWorldPosition(float lavaTime) {
    const float progress = glm::clamp(
        (lavaTime - lavaCoolStartTime) / LAVA_COOL_DURATION, 0.0f, 1.0f);
    const float distance = glm::mix(stats.riverOuterRadius,
                                    stats.riverInnerRadius, progress);

    const float terrainWidth = 1350.0f;
    const float riverNoiseX = terrainWidth * 0.5f * 0.03f;
    const float zFromVolcano = distance;
    const float rawCenterX = riverNoiseX * std::sin(zFromVolcano * 0.02f + 1.37f);
    const float startRadius = stats.craterRadius * 1.05f;
    const float startBlend = glm::smoothstep(startRadius,
                                              startRadius + startRadius * 0.6f,
                                              distance);
    const float x = glm::mix(stats.craterCenter.x,
                             stats.craterCenter.x + rawCenterX, startBlend);
    const float z = stats.craterCenter.y + zFromVolcano;
    const float y = volcano ? volcano->surfaceHeightAt(x, z) + 7.0f
                            : stats.riverEndY + 7.0f;
    return glm::vec3(x, y, z);
}

// Returns a point on (or beside) the same curved channel generated by Volcano.
// Keeping this formula in one small helper makes the scripted lightning targets
// line up with the visible river and its grassy banks.
static glm::vec3 riverWorldPoint(float alongRiver, float sideOffset,
                                 float heightOffset = 0.0f) {
    const float distance = glm::mix(stats.riverInnerRadius,
                                    stats.riverOuterRadius,
                                    glm::clamp(alongRiver, 0.0f, 1.0f));
    const float terrainHalfWidth = 1350.0f * 0.5f;
    const float rawCenterX = terrainHalfWidth * 0.03f
                           * std::sin(distance * 0.02f + 1.37f);
    const float startRadius = stats.craterRadius * 1.05f;
    const float startBlend = glm::smoothstep(startRadius,
                                              startRadius + startRadius * 0.6f,
                                              distance);
    const float centerX = glm::mix(stats.craterCenter.x,
                                   stats.craterCenter.x + rawCenterX,
                                   startBlend);
    glm::vec3 point(centerX + sideOffset, 0.0f,
                    stats.craterCenter.y + distance);
    point.y = volcano ? volcano->surfaceHeightAt(point.x, point.z) + heightOffset
                      : stats.riverEndY + heightOffset;
    return point;
}

// ------------------------------
// Crack events (CPU -> shader)
// ------------------------------
static CrackSystem gCrackSystem;
static bool gCracksCreated = false;

static const char* openGLErrorName(GLenum error)
{
    switch (error) {
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        default: return "UNKNOWN_OPENGL_ERROR";
    }
}

static void throwOnOpenGLError(const char* checkpoint)
{
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) return;

    std::ostringstream message;
    message << "OpenGL error after " << checkpoint << ": ";
    bool first = true;
    do {
        if (!first) message << ", ";
        message << openGLErrorName(error) << " (0x"
                << std::hex << std::uppercase << error << std::dec << ")";
        first = false;
        error = glGetError();
    } while (error != GL_NO_ERROR);
    throw std::runtime_error(message.str());
}

void createContext()
{
    // Load shaders
    volcanoShaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/Volcano.vertexshader",
                                       ELEMENTAL_SHADER_DIR "/Volcano.fragmentshader");
    particleShaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/ParticleShader.vertexshader",
                                        ELEMENTAL_SHADER_DIR "/ParticleShader.fragmentshader");
    depthShaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/Depth.vertexshader",
                                     ELEMENTAL_SHADER_DIR "/Depth.fragmentshader");
    propShaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/Prop.vertexshader",
                                    ELEMENTAL_SHADER_DIR "/Prop.fragmentshader");
    propMLoc = glGetUniformLocation(propShaderProgram, "M");
    propVLoc = glGetUniformLocation(propShaderProgram, "V");
    propPLoc = glGetUniformLocation(propShaderProgram, "P");
    propColorLoc = glGetUniformLocation(propShaderProgram, "uColor");
    propGrassPassLoc = glGetUniformLocation(propShaderProgram, "uGrassPass");
    propInstancedPassLoc = glGetUniformLocation(propShaderProgram, "uInstancedPass");
    propGrassGrowthLoc = glGetUniformLocation(propShaderProgram, "uGrassGrowth");
    propTimeLoc = glGetUniformLocation(propShaderProgram, "uTime");
    propCameraLocation = glGetUniformLocation(propShaderProgram, "cameraPosition_worldspace");
    propLightningLocation = glGetUniformLocation(propShaderProgram, "uLightningFlash");
    propScorchCountLocation = glGetUniformLocation(propShaderProgram, "uScorchCount");
    propScorchPositionsLocation = glGetUniformLocation(propShaderProgram, "uScorchPosXZ[0]");
    propScorchRadiiLocation = glGetUniformLocation(propShaderProgram, "uScorchRadius[0]");
    propScorchStrengthsLocation = glGetUniformLocation(propShaderProgram, "uScorchStrength[0]");
    propLightUniforms = Light::findUniformLocations(propShaderProgram, 0);

    treeShaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/TreeBillboard.vertexshader",
                                    ELEMENTAL_SHADER_DIR "/TreeBillboard.fragmentshader");
    treeVLoc = glGetUniformLocation(treeShaderProgram, "V");
    treePLoc = glGetUniformLocation(treeShaderProgram, "P");
    treeBaseLoc = glGetUniformLocation(treeShaderProgram, "uTreeBase");
    treeSizeLoc = glGetUniformLocation(treeShaderProgram, "uTreeSize");
    treeGrowthLoc = glGetUniformLocation(treeShaderProgram, "uGrowth");
    treeCameraLoc = glGetUniformLocation(treeShaderProgram, "cameraPosition_worldspace");
    treeTimeLoc = glGetUniformLocation(treeShaderProgram, "uTime");
    treeSwayLoc = glGetUniformLocation(treeShaderProgram, "uSwayPhase");
    treeTintLoc = glGetUniformLocation(treeShaderProgram, "uTint");
    treeTextureLoc = glGetUniformLocation(treeShaderProgram, "uTreeTexture");
    treeLightningLoc = glGetUniformLocation(treeShaderProgram, "uLightningFlash");

    // Get uniform locations for main shader
    projectionMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "M");
    volcanoUniforms.cameraPosition = glGetUniformLocation(
        volcanoShaderProgram, "cameraPosition_worldspace");
    volcanoUniforms.lightningFlash = glGetUniformLocation(
        volcanoShaderProgram, "u_LightningFlash");
    volcanoUniforms.shadowMap = glGetUniformLocation(volcanoShaderProgram, "u_ShadowMap");
    volcanoUniforms.lightViewProjection = glGetUniformLocation(
        volcanoShaderProgram, "u_LightVP");
    volcanoUniforms.lavaTime = glGetUniformLocation(volcanoShaderProgram, "u_LavaTime");
    volcanoUniforms.craterRadius = glGetUniformLocation(volcanoShaderProgram, "u_CraterRadius");
    volcanoUniforms.craterCenter = glGetUniformLocation(volcanoShaderProgram, "u_CraterCenter");
    volcanoUniforms.scorchCount = glGetUniformLocation(volcanoShaderProgram, "uScorchCount");
    volcanoUniforms.scorchPositions = glGetUniformLocation(
        volcanoShaderProgram, "uScorchPosXZ[0]");
    volcanoUniforms.scorchRadii = glGetUniformLocation(
        volcanoShaderProgram, "uScorchRadius[0]");
    volcanoUniforms.scorchStrengths = glGetUniformLocation(
        volcanoShaderProgram, "uScorchStrength[0]");
    volcanoUniforms.lavaCoolStart = glGetUniformLocation(
        volcanoShaderProgram, "u_LavaCoolStart");
    volcanoUniforms.lavaCoolDuration = glGetUniformLocation(
        volcanoShaderProgram, "u_LavaCoolDuration");
    volcanoUniforms.waterFill = glGetUniformLocation(volcanoShaderProgram, "u_WaterFill");
    volcanoUniforms.grassGrowth = glGetUniformLocation(volcanoShaderProgram, "u_GrassGrowth");
    volcanoUniforms.rainIntensity = glGetUniformLocation(
        volcanoShaderProgram, "u_RainIntensity");
    volcanoUniforms.riverInnerRadius = glGetUniformLocation(
        volcanoShaderProgram, "u_RiverInnerRadius");
    volcanoUniforms.riverOuterRadius = glGetUniformLocation(
        volcanoShaderProgram, "u_RiverOuterRadius");
    volcanoUniforms.electricStart = glGetUniformLocation(
        volcanoShaderProgram, "u_ElectricStart");
    volcanoUniforms.electricDuration = glGetUniformLocation(
        volcanoShaderProgram, "u_ElectricDuration");
    volcanoUniforms.electricStrikeDistance = glGetUniformLocation(
        volcanoShaderProgram, "u_ElectricStrikeDist");
    volcanoUniforms.materialAmbient = glGetUniformLocation(volcanoShaderProgram, "Ka");
    volcanoUniforms.materialSpecular = glGetUniformLocation(volcanoShaderProgram, "Ks");
    volcanoUniforms.materialShininess = glGetUniformLocation(volcanoShaderProgram, "Ns");
    volcanoLightUniforms = Light::findUniformLocations(volcanoShaderProgram, 0);

    // Depth (shadow) pass uniforms
    depthMLocation = glGetUniformLocation(depthShaderProgram, "M");
    depthVLocation = glGetUniformLocation(depthShaderProgram, "V");
    depthPLocation = glGetUniformLocation(depthShaderProgram, "P");
    depthLavaTimeLocation = glGetUniformLocation(depthShaderProgram, "u_LavaTime");
    depthCraterRadiusLocation = glGetUniformLocation(depthShaderProgram, "u_CraterRadius");

    // Shadow map depth texture + FBO (no color attachment needed)
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // depth=1.0 (far) outside the frustum
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::ostringstream message;
        message << "Shadow map framebuffer is incomplete (0x"
                << std::hex << std::uppercase << framebufferStatus << ")";
        throw std::runtime_error(message.str());
    }

    // Particle shader uniforms
    particlePVLocation = glGetUniformLocation(particleShaderProgram, "PV");
    particleTextureSampler = glGetUniformLocation(particleShaderProgram, "texture0");
    particleTintLocation = glGetUniformLocation(particleShaderProgram, "uTint");
    particleAlphaLocation = glGetUniformLocation(particleShaderProgram, "uAlphaScale");
    particleShapeScaleLocation = glGetUniformLocation(particleShaderProgram, "uShapeScale");
    particlePuffinessLocation = glGetUniformLocation(particleShaderProgram, "uPuffiness");
    particleShearLocation = glGetUniformLocation(particleShaderProgram, "uShear");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Create volcano
    volcano = new Volcano(512, 1350.0f, 20.0f, glm::vec2(0.0f, -400.0f));
    stats = volcano->getStats();

    // Load terrain texture
    terrainTexture = loadSOIL(ELEMENTAL_ASSET_DIR "/Diffusemap.png");


    terrainTextureSampler = glGetUniformLocation(volcanoShaderProgram, "uTerrainTexture");


    glBindTexture(GL_TEXTURE_2D, terrainTexture);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // skybox
    std::vector<std::string> cubemapFaces = {
        ELEMENTAL_SKYBOX_DIR "/right.png",
        ELEMENTAL_SKYBOX_DIR "/left.png",
        ELEMENTAL_SKYBOX_DIR "/top.png",
        ELEMENTAL_SKYBOX_DIR "/bottom.png",
        ELEMENTAL_SKYBOX_DIR "/front.png",
        ELEMENTAL_SKYBOX_DIR "/back.png"
    };
    skybox = new Skybox(cubemapFaces);

    sceneDirector = new SceneDirector(camera, moonlight, volcano, skybox);
    sceneDirector->setTimeScale(gInitialTimeScale);
    lightningSystem = new LightningSystem();

    // Procedural vegetation geometry (Part B.4d). The whole grass field is one
    // mesh/draw call; flowers reuse the small sphere and cylinder primitives.
    propSphere = createUnitSphereDrawable();
    propCylinder = createUnitCylinderDrawable();
    propSphereRenderer = new InstancedPropRenderer(propSphere);
    propCylinderRenderer = new InstancedPropRenderer(propCylinder);
    grassField = createGrassFieldDrawable(*volcano);

    // One flower has one stem and six bloom pieces. Reserve once so the
    // per-frame animation updates do not repeatedly allocate memory.
    cylinderPropMatrices.reserve(96);
    cylinderPropColors.reserve(96);
    spherePropMatrices.reserve(512);
    spherePropColors.reserve(512);

    // Flat, camera-facing quad used for ALL particle emitters. A textured billboard
    // quad reads as a smoke puff / raindrop; a sphere (the old particle model) does not.
    // Corners span [-1,1] so the per-particle scale (mass) maps directly to world size.
    std::vector<glm::vec3> quadVerts = {
        {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}
    };
    std::vector<glm::vec2> quadUVs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };
    std::vector<glm::vec3> quadNormals(6, glm::vec3(0.0f, 0.0f, 1.0f));
    particleQuad = new Drawable(quadVerts, quadUVs, quadNormals, false);

    smokeTexture = loadSOIL(ELEMENTAL_ASSET_DIR "/smoke3.png");
    treeTexture = loadSOILWithAlpha(ELEMENTAL_ASSET_DIR "/tree_billboard.png");
    almondTreeTexture = loadSOILWithAlpha(
        ELEMENTAL_ASSET_DIR "/almond_tree_billboard.png");

    // Keep the initial run and every restart on the same procedural sequence.
    // The particle motion still evolves naturally, but its starting layout is repeatable.
    std::srand(SIMULATION_RANDOM_SEED);
    gLightningRandom.seed(SIMULATION_RANDOM_SEED + 1u);

    // Smoke: spawn at crater and start emitting after the shake window ends (lava starts)
    glm::vec3 craterPos(stats.craterCenter.x, stats.craterTop - 132.0f, stats.craterCenter.y);
    smokeEmitter = new SmokeEmitter(
        particleQuad, 760, craterPos,
        38.0f,       // broad source across the crater mouth
        10.0f, 38.0f, // large overlapping puffs, not visible specks
        34.0f, 14.0f,
        0.075f);

    // A wide, sparse pre-existing bank spans the sky instead of collecting in
    // one small spot. The denser storm cloud is still created by the domino chain.
    ambientCloudEmitter = new CloudEmitter(
        particleQuad, 72, glm::vec3(-75.0f, 480.0f, -470.0f),
        540.0f, 88.0f, 22.0f, 46.0f, 0.55f, 3.2f);

    throwOnOpenGLError("scene resource creation");
}

void resetSimulation(double realTimeSeconds)
{
    delete smokeEmitter;
    delete calmSmokeEmitter;
    delete ambientCloudEmitter;
    delete ashEmitter;
    delete cloudEmitter;
    delete rainEmitter;
    smokeEmitter = nullptr;
    calmSmokeEmitter = nullptr;
    ambientCloudEmitter = nullptr;
    ashEmitter = nullptr;
    cloudEmitter = nullptr;
    rainEmitter = nullptr;

    // Seed before constructing emitters: their constructors generate the initial
    // particle layout, so seeding afterwards would make restarts visibly differ.
    std::srand(SIMULATION_RANDOM_SEED);
    gLightningRandom.seed(SIMULATION_RANDOM_SEED + 1u);

    const glm::vec3 craterPos(stats.craterCenter.x, stats.craterTop - 132.0f,
                              stats.craterCenter.y);
    smokeEmitter = new SmokeEmitter(
        particleQuad, 760, craterPos,
        38.0f, 10.0f, 38.0f, 34.0f, 14.0f, 0.075f);
    ambientCloudEmitter = new CloudEmitter(
        particleQuad, 72, glm::vec3(-75.0f, 480.0f, -470.0f),
        540.0f, 88.0f, 22.0f, 46.0f, 0.55f, 3.2f);

    lavaCoolStartTime = -1.0f;
    rainStartTime = -1.0f;
    floraGrowthStartTime = -1.0f;
    nextLightningTime = -1.0f;
    manualLightningRequested = false;
    earlyGroundStrikeCount = 0;
    vegetationGroundStrikeCount = 0;
    riverStrikeInFlight = false;
    electricRiverStartTime = -1.0f;
    electricRiverStrikeDistance = 0.0f;
    floraSpawned = false;
    floraProps.clear();
    treeProps.clear();
    scorchPositions.clear();
    scorchRadii.clear();
    scorchStrengths.clear();
    gCrackSystem.reset();
    gCracksCreated = false;
    if (lightningSystem) lightningSystem->reset();
    if (sceneDirector) sceneDirector->reset(realTimeSeconds);
    if (camera) camera->resetToEstablishingShot();
}

void free()
{
    delete sceneDirector;
    sceneDirector = nullptr;

#if defined(ELEMENTAL_ENABLE_IMGUI)
    if (gImGuiOpenGLInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        gImGuiOpenGLInitialized = false;
    }
    if (gImGuiGlfwInitialized) {
        ImGui_ImplGlfw_Shutdown();
        gImGuiGlfwInitialized = false;
    }
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
#endif

    if (gOpenGLContextReady) {
        glDeleteProgram(volcanoShaderProgram);
        glDeleteProgram(particleShaderProgram);
        glDeleteProgram(depthShaderProgram);
        glDeleteProgram(propShaderProgram);
        glDeleteProgram(treeShaderProgram);
        volcanoShaderProgram = particleShaderProgram = depthShaderProgram = 0;
        propShaderProgram = treeShaderProgram = 0;

        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthMapTexture);
        depthMapFBO = depthMapTexture = 0;

        delete volcano;
        delete skybox;
        delete smokeEmitter;
        delete calmSmokeEmitter;
        delete ambientCloudEmitter;
        delete ashEmitter;
        delete cloudEmitter;
        delete rainEmitter;
        delete lightningSystem;
        delete propSphereRenderer;
        delete propCylinderRenderer;
        delete propSphere;
        delete propCylinder;
        delete grassField;
        delete particleQuad;
        volcano = nullptr;
        skybox = nullptr;
        smokeEmitter = nullptr;
        calmSmokeEmitter = nullptr;
        ambientCloudEmitter = nullptr;
        ashEmitter = nullptr;
        cloudEmitter = nullptr;
        rainEmitter = nullptr;
        lightningSystem = nullptr;
        propSphereRenderer = nullptr;
        propCylinderRenderer = nullptr;
        propSphere = nullptr;
        propCylinder = nullptr;
        grassField = nullptr;
        particleQuad = nullptr;

        glDeleteTextures(1, &smokeTexture);
        glDeleteTextures(1, &treeTexture);
        glDeleteTextures(1, &almondTreeTexture);
        glDeleteTextures(1, &terrainTexture);
        smokeTexture = treeTexture = almondTreeTexture = terrainTexture = 0;
    }

    delete moonlight;
    delete camera;
    moonlight = nullptr;
    camera = nullptr;

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
    gOpenGLContextReady = false;
}

void mainLoop()
{
    double lastTime = glfwGetTime();
    const double performanceStartTime = lastTime;
    unsigned long long renderedFrameCount = 0;
    std::vector<float> frameTimesMs;
    if (gReportPerformance) frameTimesMs.reserve(4096);
    bool firstFrameDiagnostics = true;

    do
    {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;
        if (gReportPerformance && renderedFrameCount > 0) {
            frameTimesMs.push_back(deltaTime * 1000.0f);
        }
        int framebufferWidth = W_WIDTH;
        int framebufferHeight = W_HEIGHT;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

#if defined(ELEMENTAL_ENABLE_IMGUI)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif

        // --- Time controller (Part B.5): P pauses/resumes, [ / ] slow down / speed up,
        // 0 resets to 1x. Edge-triggered so a held key doesn't repeat every frame.
        if (sceneDirector) {
            static bool pKeyWasDown = false;
            bool pKeyIsDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
            if (pKeyIsDown && !pKeyWasDown) {
                sceneDirector->setPaused(!sceneDirector->isPaused());
            }
            pKeyWasDown = pKeyIsDown;

            static bool slowKeyWasDown = false;
            bool slowKeyIsDown = glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
            if (slowKeyIsDown && !slowKeyWasDown) {
                sceneDirector->setTimeScale(sceneDirector->getTimeScale() * 0.5f);
            }
            slowKeyWasDown = slowKeyIsDown;

            static bool fastKeyWasDown = false;
            bool fastKeyIsDown = glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
            if (fastKeyIsDown && !fastKeyWasDown) {
                sceneDirector->setTimeScale(sceneDirector->getTimeScale() * 2.0f);
            }
            fastKeyWasDown = fastKeyIsDown;

            static bool resetKeyWasDown = false;
            bool resetKeyIsDown = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;
            if (resetKeyIsDown && !resetKeyWasDown) {
                sceneDirector->setTimeScale(1.0f);
            }
            resetKeyWasDown = resetKeyIsDown;

            static bool restartKeyWasDown = false;
            bool restartKeyIsDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
            if (restartKeyIsDown && !restartKeyWasDown) {
                resetSimulation(currentTime);
            }
            restartKeyWasDown = restartKeyIsDown;

            static bool cameraResetKeyWasDown = false;
            bool cameraResetKeyIsDown = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
            if (cameraResetKeyIsDown && !cameraResetKeyWasDown) {
                camera->resetToEstablishingShot();
            }
            cameraResetKeyWasDown = cameraResetKeyIsDown;

            static bool lightningKeyWasDown = false;
            bool lightningKeyIsDown = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
            if (lightningKeyIsDown && !lightningKeyWasDown) {
                manualLightningRequested = true;
            }
            lightningKeyWasDown = lightningKeyIsDown;

#if defined(ELEMENTAL_ENABLE_IMGUI)
            static bool uiKeyWasDown = false;
            bool uiKeyIsDown = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
            if (uiKeyIsDown && !uiKeyWasDown) {
                gShowHud = !gShowHud;
                camera->setMouseLookEnabled(!gShowHud);
            }
            uiKeyWasDown = uiKeyIsDown;
#endif
        }

        if (sceneDirector) {
            sceneDirector->update(currentTime, deltaTime);
        } else if (camera) {
            camera->update();
        }

        const float simulationTime = sceneDirector
            ? static_cast<float>(sceneDirector->getSimSeconds())
            : static_cast<float>(currentTime);
        const float simulationDelta = sceneDirector
            ? sceneDirector->getScaledDeltaSeconds(deltaTime)
            : deltaTime;

        // Every physical effect consumes scaled simulation time. Pausing now
        // freezes smoke, rain and lightning motion as well as the event timeline.
        // SceneDirector defines lavaTimeSeconds >= 0 at m_lavaStartSeconds (end of shake).
        float lavaTimeForSmoke = sceneDirector ? sceneDirector->getLavaTimeSeconds()
                                               : static_cast<float>(currentTime);
        if (smokeEmitter && sceneDirector && lavaTimeForSmoke >= 0.0f) {
            smokeEmitter->updateParticles(simulationTime, simulationDelta, camera->position);
        }
        if (calmSmokeEmitter) {
            calmSmokeEmitter->updateParticles(simulationTime, simulationDelta,
                                              camera->position);
        }
        if (ambientCloudEmitter) {
            ambientCloudEmitter->updateParticles(simulationTime, simulationDelta, camera->position);
        }
        if (ashEmitter) {
            // Recycle particles at the moving end-to-start cooling boundary.
            // Older puffs remain behind, so the smoke clearly traces the same
            // progressive transformation visible on the lava surface.
            if (lavaCoolStartTime >= 0.0f && lavaTimeForSmoke >= 0.0f) {
                ashEmitter->followMovingSource(
                    coolingFrontWorldPosition(lavaTimeForSmoke),
                    simulationDelta,
                    210.0f);
            }
            ashEmitter->updateParticles(simulationTime, simulationDelta, camera->position);
        }
        if (cloudEmitter) {
            cloudEmitter->updateParticles(simulationTime, simulationDelta, camera->position);
        }
        if (rainEmitter) {
            rainEmitter->updateParticles(simulationTime, simulationDelta, camera->position);
        }
        if (lightningSystem) lightningSystem->update(simulationTime, simulationDelta);

        // Update light
        moonlight->update();

        float calmProgress = 0.0f;
        if (sceneDirector) {
            if (sceneDirector->getStage() == SimulationStage::CalmNight) {
                calmProgress = glm::smoothstep(0.0f, CALM_NIGHT_DURATION,
                                                sceneDirector->getStageElapsedSeconds());
            } else if (sceneDirector->getStage() == SimulationStage::Complete) {
                calmProgress = 1.0f;
            }
        }

        // The ending stays nocturnal, but clearer air allows slightly warmer,
        // cleaner moonlight to reveal the recovered landscape.
        moonlight->La = glm::mix(glm::vec4(0.040f, 0.040f, 0.200f, 1.0f),
                                 glm::vec4(0.052f, 0.058f, 0.175f, 1.0f),
                                 calmProgress);
        moonlight->Ld = glm::mix(glm::vec4(0.34f, 0.50f, 0.76f, 1.0f),
                                 glm::vec4(0.40f, 0.53f, 0.72f, 1.0f),
                                 calmProgress);
        moonlight->Ls = glm::mix(glm::vec4(0.68f, 0.77f, 0.90f, 1.0f),
                                 glm::vec4(0.74f, 0.81f, 0.91f, 1.0f),
                                 calmProgress);

        float lavaTime = sceneDirector ? sceneDirector->getLavaTimeSeconds()
                                       : static_cast<float>(currentTime);

        // --- Shadow pass: render the volcano's depth from the moonlight's POV ---
        {
            mat4 lightView = moonlight->viewMatrix;
            mat4 lightProjection = moonlight->projectionMatrix;
            mat4 identity = mat4(1.0);

            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
            glClear(GL_DEPTH_BUFFER_BIT);

            glUseProgram(depthShaderProgram);
            glUniformMatrix4fv(depthMLocation, 1, GL_FALSE, &identity[0][0]);
            glUniformMatrix4fv(depthVLocation, 1, GL_FALSE, &lightView[0][0]);
            glUniformMatrix4fv(depthPLocation, 1, GL_FALSE, &lightProjection[0][0]);
            glUniform1f(depthLavaTimeLocation, lavaTime);
            glUniform1f(depthCraterRadiusLocation, stats.craterRadius);

            volcano->Draw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // GLFW window units and framebuffer pixels differ on Retina/HiDPI
            // displays, so always restore the actual drawable size.
            glViewport(0, 0, framebufferWidth, framebufferHeight);
        }

        if (firstFrameDiagnostics) {
            throwOnOpenGLError("first shadow pass");
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        mat4 modelMatrix = mat4(1.0);

        // Draw skybox first
        if (skybox)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            skybox->Draw(viewMatrix, projectionMatrix,
                         lightningSystem ? lightningSystem->flashStrength() : 0.0f,
                         simulationTime,
                         // The moon disc is composed for the establishing shot.
                         // The terrain light and shadow pass still share one
                         // physically consistent directional-light vector.
                         glm::vec3(-0.35f, 0.15f, -0.925f),
                         calmProgress);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glUseProgram(volcanoShaderProgram);

        // transfer uniforms to GPU
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniform3fv(volcanoUniforms.cameraPosition, 1, &camera->position[0]);
        glUniform1f(volcanoUniforms.lightningFlash,
                    lightningSystem ? lightningSystem->flashStrength() : 0.0f);

        // Bind terrain texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrainTexture);
        glUniform1i(terrainTextureSampler, 0);

        // Bind shadow map (populated by the depth pass above)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(volcanoUniforms.shadowMap, 1);
        mat4 lightVP = moonlight->lightVP();
        glUniformMatrix4fv(volcanoUniforms.lightViewProjection,
                           1, GL_FALSE, &lightVP[0][0]);

        // Send time uniform
        glUniform1f(volcanoUniforms.lavaTime, lavaTime);

        // Create cracks once shortly before lava starts flowing.
        // (SceneDirector starts lava at 7 seconds, so lavaTime becomes >= 0 then.
        // We create cracks when lavaTime becomes close to 0.)
        if (!gCracksCreated && lavaTime > -1.0f) {
            stats = volcano->getStats();

            // Hardcoded crack seed points near the volcano base.
            // Volcano center is approximately (0, -400) in XZ.
            // For line cracks: set direction radially outward from the center.
            auto radialDir = [&](const glm::vec2& p) {
                glm::vec2 d = p - stats.craterCenter;
                float len = glm::length(d);
                if (len < 1e-5f) return glm::vec2(1.0f, 0.0f);
                return d / len;
            };

            gCrackSystem.reset();
            {
                glm::vec2 p(-180.0f, 80.0f);
                gCrackSystem.addCrack(p, lavaTime + 0.30f, 1.4f, 80.0f, radialDir(p), 130.0f);
            }
            {
                glm::vec2 p(320.0f, -240.0f);
                gCrackSystem.addCrack(p, lavaTime + 0.00f, 1.9f, 95.0f, radialDir(p), 160.0f);
            }
            {
                glm::vec2 p(-200.0f, -340.0f);
                gCrackSystem.addCrack(p, lavaTime + 0.15f, 1.6f, 85.0f, radialDir(p), 140.0f);
            }
            {
                glm::vec2 p(-170.0f, -180.0f);
                gCrackSystem.addCrack(p, lavaTime + 0.45f, 1.7f, 90.0f, radialDir(p), 150.0f);
            }
            {
                glm::vec2 p(180.0f, -60.0f);
                gCrackSystem.addCrack(p, lavaTime + 0.60f, 1.1f, 75.0f, radialDir(p), 120.0f);
            }

            gCracksCreated = true;
        }

        // Crack data changes only at creation/reset; unchanged array uploads are
        // skipped by CrackSystem on later frames.
        gCrackSystem.uploadToVolcanoShader(volcanoShaderProgram);

        VolcanoStats stats = volcano->getStats();
        glUniform1f(volcanoUniforms.craterRadius, stats.craterRadius);
        glUniform2f(volcanoUniforms.craterCenter,
                    stats.craterCenter.x, stats.craterCenter.y);

        // State-driven domino chain. Each transition creates the next visible
        // effect once, then the stage timer controls the following transition.
        if (sceneDirector->getStage() == SimulationStage::LavaFlowing && lavaTime >= 0.0f) {
            float flowHead = lavaTime * LAVA_FLOW_SPEED;
            if (flowHead >= stats.riverOuterRadius) {
                lavaCoolStartTime = lavaTime;

                glm::vec3 riverEndPos = coolingFrontWorldPosition(lavaTime);
                ashEmitter = new SmokeEmitter(
                    particleQuad, 1300, riverEndPos,
                    14.0f,    // compact band around the cooling boundary
                    3.0f, 11.0f,
                    38.0f, 12.0f,
                    0.12f);
                ashEmitter->plumeHeight = 420.0f;
                sceneDirector->transitionTo(SimulationStage::SmokeAndAsh);
            }
        }

        if (sceneDirector->getStage() == SimulationStage::SmokeAndAsh &&
            sceneDirector->getStageElapsedSeconds() >= CLOUD_FORM_DELAY) {
            // The ash gathers into a broad layered bank over the river/volcano
            // corridor, rather than one compact ball at the old impact point.
            glm::vec3 cloudPos(
                glm::mix(stats.craterCenter.x, stats.riverEndXZ.x, 0.48f),
                stats.craterTop + 230.0f,
                glm::mix(stats.craterCenter.y, stats.riverEndXZ.y, 0.48f));
            cloudEmitter = new CloudEmitter(
                particleQuad, 110, cloudPos,
                285.0f, 72.0f, 22.0f, 50.0f, 1.15f, 5.0f);
            sceneDirector->transitionTo(SimulationStage::CloudFormation);
            nextLightningTime = simulationTime + LIGHTNING_START_DELAY;
        }

        if (sceneDirector->getStage() == SimulationStage::CloudFormation &&
            sceneDirector->getStageElapsedSeconds() >= RAIN_START_DELAY) {
            rainStartTime = lavaTime;
            rainEmitter = new RainEmitter(particleQuad, 1200, cloudEmitter->emitter_pos,
                                          stats.riverEndY, 285.0f);
            sceneDirector->transitionTo(SimulationStage::Raining);
        }

        if (sceneDirector->getStage() == SimulationStage::Raining &&
            sceneDirector->getStageElapsedSeconds() >= 1.5f) {
            sceneDirector->transitionTo(SimulationStage::RiverFilling);
        }

        // The storm is finite: at most three early strikes establish the
        // weather, then three deliberate meadow strikes happen after the
        // vegetation has grown. B remains a manual demonstration shortcut.
        CloudEmitter* lightningCloud = cloudEmitter ? cloudEmitter : ambientCloudEmitter;
        const SimulationStage currentStage = sceneDirector->getStage();
        const bool earlyStormStage = currentStage == SimulationStage::CloudFormation ||
                                     currentStage == SimulationStage::Raining ||
                                     currentStage == SimulationStage::RiverFilling ||
                                     currentStage == SimulationStage::VegetationGrowing;
        const bool automaticEarlyStrike = cloudEmitter && earlyStormStage &&
            earlyGroundStrikeCount < 3 && nextLightningTime >= 0.0f &&
            simulationTime >= nextLightningTime && !lightningSystem->isActive();
        const bool automaticMeadowStrike = cloudEmitter &&
            currentStage == SimulationStage::LightningStorm &&
            vegetationGroundStrikeCount < 3 && nextLightningTime >= 0.0f &&
            simulationTime >= nextLightningTime && !lightningSystem->isActive();

        if (lightningCloud &&
            (manualLightningRequested || automaticEarlyStrike || automaticMeadowStrike)) {
            glm::vec3 strike;
            if (automaticMeadowStrike) {
                // Alternating banks make the new black patches readable across
                // the grown meadow instead of stacking all strikes in one spot.
                const float alongTargets[] = { 0.76f, 0.86f, 0.94f };
                const float sideTargets[] = { -180.0f, 205.0f, -125.0f };
                const int target = vegetationGroundStrikeCount;
                strike = riverWorldPoint(alongTargets[target], sideTargets[target], 2.0f);
                ++vegetationGroundStrikeCount;
                nextLightningTime = simulationTime + 3.2f;
            } else {
                const float strikeX = lightningCloud->emitter_pos.x - 110.0f
                                    + lightningRandom01() * 220.0f;
                const float strikeZ = lightningCloud->emitter_pos.z - 70.0f
                                    + lightningRandom01() * 140.0f;
                strike = glm::vec3(strikeX,
                                   volcano->surfaceHeightAt(strikeX, strikeZ) + 2.0f,
                                   strikeZ);
                if (automaticEarlyStrike) {
                    ++earlyGroundStrikeCount;
                    nextLightningTime = simulationTime + 7.0f;
                }
            }
            lightningSystem->trigger(simulationTime, lightningCloud->emitter_pos, strike);
            manualLightningRequested = false;
        }

        // Once the meadow has visibly taken three hits, aim the final bolt at
        // the centre of the full river. Its impact starts the mixing effect.
        if (cloudEmitter && currentStage == SimulationStage::LightningStorm &&
            vegetationGroundStrikeCount >= 3 &&
            sceneDirector->getStageElapsedSeconds() >= 10.0f &&
            !riverStrikeInFlight && !lightningSystem->isActive()) {
            const float riverTargetAlong = 0.72f;
            const glm::vec3 riverStrike = riverWorldPoint(riverTargetAlong, 0.0f, 2.4f);
            electricRiverStrikeDistance = glm::mix(stats.riverInnerRadius,
                                                    stats.riverOuterRadius,
                                                    riverTargetAlong);
            riverStrikeInFlight = true;
            nextLightningTime = -1.0f;
            lightningSystem->trigger(simulationTime, cloudEmitter->emitter_pos, riverStrike);
        }

        if (lightningSystem->consumeImpact()) {
            const glm::vec3 strike = lightningSystem->strikePosition();
            if (riverStrikeInFlight) {
                riverStrikeInFlight = false;
                electricRiverStartTime = lavaTime;
                sceneDirector->transitionTo(SimulationStage::ElectrifiedRiver);
            } else if (scorchPositions.size() < 8) {
                scorchPositions.push_back(glm::vec2(strike.x, strike.z));
                scorchRadii.push_back(50.0f + lightningRandom01() * 24.0f);
                scorchStrengths.push_back(0.9f + lightningRandom01() * 0.1f);
            }
        }
        glUniform1i(volcanoUniforms.scorchCount,
                    static_cast<GLint>(scorchPositions.size()));
        if (!scorchPositions.empty()) {
            glUniform2fv(volcanoUniforms.scorchPositions,
                         static_cast<GLsizei>(scorchPositions.size()), &scorchPositions[0].x);
            glUniform1fv(volcanoUniforms.scorchRadii,
                         static_cast<GLsizei>(scorchRadii.size()), &scorchRadii[0]);
            glUniform1fv(volcanoUniforms.scorchStrengths,
                         static_cast<GLsizei>(scorchStrengths.size()), &scorchStrengths[0]);
        }
        glUniform1f(volcanoUniforms.lavaCoolStart, lavaCoolStartTime);
        glUniform1f(volcanoUniforms.lavaCoolDuration, LAVA_COOL_DURATION);

        // Domino chain (Part B.4c): river channel fills with water as rain falls.
        float waterFill = 0.0f;
        if (rainEmitter && rainStartTime >= 0.0f) {
            waterFill = glm::clamp((lavaTime - rainStartTime) / WATER_FILL_DURATION, 0.0f, 1.0f);
        }
        glUniform1f(volcanoUniforms.waterFill, waterFill);
        // The order is intentional: the river must be completely full before
        // any grass appears. floraGrowthStartTime is set by the 100% trigger below.
        const float grassAge = floraSpawned
            ? glm::max(0.0f, lavaTime - floraGrowthStartTime)
            : 0.0f;
        const float grassGrowth = floraSpawned
            ? glm::smoothstep(0.0f, GRASS_GROW_DURATION, grassAge)
            : 0.0f;
        glUniform1f(volcanoUniforms.grassGrowth, grassGrowth);
        float rainIntensity = rainEmitter && rainStartTime >= 0.0f
            ? glm::smoothstep(0.0f, 1.6f, lavaTime - rainStartTime)
            : 0.0f;
        if (floraSpawned) {
            rainIntensity *= 1.0f - glm::smoothstep(0.0f, 5.0f,
                                                    lavaTime - floraGrowthStartTime);
        }
        glUniform1f(volcanoUniforms.rainIntensity, rainIntensity);
        glUniform1f(volcanoUniforms.riverInnerRadius, stats.riverInnerRadius);
        glUniform1f(volcanoUniforms.riverOuterRadius, stats.riverOuterRadius);
        glUniform1f(volcanoUniforms.electricStart, electricRiverStartTime);
        glUniform1f(volcanoUniforms.electricDuration, ELECTRIC_RIVER_DURATION);
        glUniform1f(volcanoUniforms.electricStrikeDistance, electricRiverStrikeDistance);

        // Domino chain (Part B.4d): the broad grass corridor has finished growing
        // when the river becomes full. Small flowers then appear along both banks;
        // trees are deliberately left for the next vegetation pass.
        if (!floraSpawned && waterFill >= 1.0f) {
            floraSpawned = true;
            floraGrowthStartTime = lavaTime;
            sceneDirector->transitionTo(SimulationStage::VegetationGrowing);

            // Placement must not depend on how many per-frame random values the
            // particle systems consumed. Recreate the same local sequence for
            // every run and restart, regardless of FPS or time scale.
            std::mt19937 floraRandom(SIMULATION_RANDOM_SEED + 2u);
            std::uniform_real_distribution<float> floraDistribution(0.0f, 1.0f);
            auto floraRandom01 = [&]() {
                return floraDistribution(floraRandom);
            };

            auto addFloraPart = [&](const glm::vec3& position,
                                    const glm::vec3& scale,
                                    const glm::vec3& color,
                                    float groundY,
                                    float delay,
                                    float duration,
                                    float swayAmount,
                                    bool useCylinder) {
                floraProps.push_back({ position, scale, color, groundY, delay,
                                       duration, floraRandom01() * 6.2831853f, swayAmount,
                                       useCylinder });
            };

            // Sample the same curved centre line used to carve the river, then
            // move a short distance left or right. Sampling the generated height
            // field keeps every stem rooted on its local bank instead of floating.
            auto riverBankSpot = [&](float alongRiver, float sideOffset) {
                float distance = glm::mix(stats.riverInnerRadius,
                                          stats.riverOuterRadius, alongRiver);
                const float terrainHalfWidth = 1350.0f * 0.5f;
                float rawCenterX = terrainHalfWidth * 0.03f
                                 * std::sin(distance * 0.02f + 1.37f);
                float startRadius = stats.craterRadius * 1.05f;
                float startBlend = glm::smoothstep(startRadius,
                                                    startRadius + startRadius * 0.6f,
                                                    distance);
                float centerX = glm::mix(stats.craterCenter.x,
                                         stats.craterCenter.x + rawCenterX,
                                         startBlend);
                glm::vec3 point(centerX + sideOffset,
                                0.0f,
                                stats.craterCenter.y + distance);
                point.y = volcano->surfaceHeightAt(point.x, point.z);
                return point;
            };

            const glm::vec3 flowerColors[] = {
                glm::vec3(0.94f, 0.10f, 0.12f), // poppy red
                glm::vec3(0.96f, 0.70f, 0.08f), // warm yellow
                glm::vec3(0.96f, 0.34f, 0.58f)  // natural pink
            };
            const int FLOWER_COUNT = 72;
            for (int i = 0; i < FLOWER_COUNT; ++i) {
                // Keep flowers on the lower, grassy part of the river. Squaring
                // the spread value leaves most blooms near the bank while still
                // scattering some naturally farther into the meadow.
                float alongRiver = 0.34f + floraRandom01() * 0.60f;
                float bankSide = (i % 2 == 0) ? -1.0f : 1.0f;
                float meadowSpread = floraRandom01();
                float sideOffset = bankSide
                                 * (34.0f + meadowSpread * meadowSpread * 165.0f);
                glm::vec3 pos = riverBankSpot(alongRiver, sideOffset);
                float stemHeight = 5.5f + floraRandom01() * 3.0f;
                // Flowers wait until the grass has completed its own growth.
                float delay = GRASS_GROW_DURATION + 0.7f
                            + floraRandom01() * 2.8f;
                // Cycling the palette guarantees a natural balance instead of
                // allowing one random color to dominate a particular run.
                glm::vec3 petalColor = flowerColors[i % 3];
                addFloraPart(pos + glm::vec3(0.0f, stemHeight * 0.5f, 0.0f),
                             glm::vec3(0.42f, stemHeight * 0.5f, 0.42f),
                             glm::vec3(0.09f, 0.32f, 0.08f),
                             pos.y, delay, 1.5f, 0.10f, true);

                glm::vec3 bloomCenter = pos + glm::vec3(0.0f, stemHeight + 0.9f, 0.0f);
                addFloraPart(bloomCenter, glm::vec3(1.35f, 0.88f, 1.35f),
                             glm::vec3(0.96f, 0.66f, 0.06f),
                             pos.y, delay + 0.55f, 1.5f, 0.28f, false);
                for (int petal = 0; petal < 5; ++petal) {
                    float a = float(petal) * 1.2566371f;
                    glm::vec3 petalOffset(std::cos(a) * 2.05f,
                                          0.14f,
                                          std::sin(a) * 2.05f);
                    addFloraPart(bloomCenter + petalOffset,
                                 glm::vec3(1.65f, 0.60f, 1.12f),
                                 petalColor, pos.y,
                                 delay + 0.67f + petal * 0.045f,
                                 1.6f, 0.32f, false);
                }
            }

            // A few medium trees grow farther into the meadow after the flowers.
            // They use camera-facing cutout billboards, not primitive geometry.
            const glm::vec3 treeTints[] = {
                glm::vec3(0.92f, 1.00f, 0.91f),
                glm::vec3(0.82f, 0.94f, 0.80f),
                glm::vec3(1.00f, 0.95f, 0.82f)
            };
            const int TREE_COUNT = 18;
            for (int i = 0; i < TREE_COUNT; ++i) {
                float alongRiver = 0.42f + floraRandom01() * 0.54f;
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                float sideOffset = side * (135.0f + floraRandom01() * 235.0f);
                glm::vec3 base = riverBankSpot(alongRiver, sideOffset);
                float height = 52.0f + floraRandom01() * 19.0f;
                float width = height * (0.70f + floraRandom01() * 0.10f);
                const bool flowering = (i % 3) == 1;
                const glm::vec3 tint = flowering
                    ? glm::vec3(0.96f, 0.94f, 0.91f)
                    : treeTints[i % 3];
                treeProps.push_back({ base, glm::vec2(width, height),
                                      tint,
                                      TREE_GROW_DELAY + floraRandom01() * 2.0f,
                                      floraRandom01() * 6.2831853f,
                                      flowering });
            }
        }

        if (sceneDirector->getStage() == SimulationStage::VegetationGrowing &&
            sceneDirector->getStageElapsedSeconds() >= 18.5f) {
            // Let the completed meadow remain on screen while several bolts
            // char the real grass before the final water strike.
            // Earlier storm scars are cleared here: otherwise old black masks
            // become newly conspicuous as green blades grow, falsely suggesting
            // damage without a simultaneous visible strike.
            scorchPositions.clear();
            scorchRadii.clear();
            scorchStrengths.clear();
            nextLightningTime = simulationTime + 1.0f;
            vegetationGroundStrikeCount = 0;
            sceneDirector->transitionTo(SimulationStage::LightningStorm);
        }

        if (sceneDirector->getStage() == SimulationStage::ElectrifiedRiver &&
            sceneDirector->getStageElapsedSeconds() >= ELECTRIC_RIVER_DURATION + 0.8f) {
            if (!calmSmokeEmitter) {
                const glm::vec3 craterPos(stats.craterCenter.x,
                                           stats.craterTop - 132.0f,
                                           stats.craterCenter.y);
                calmSmokeEmitter = new SmokeEmitter(
                    particleQuad, 170, craterPos,
                    22.0f, 5.0f, 18.0f,
                    7.0f, 4.0f, 0.055f);
                calmSmokeEmitter->plumeHeight = 190.0f;
                calmSmokeEmitter->turbulence = 0.20f;
            }
            sceneDirector->transitionTo(SimulationStage::CalmNight);
        }

        if (sceneDirector->getStage() == SimulationStage::CalmNight &&
            sceneDirector->getStageElapsedSeconds() >= CALM_NIGHT_DURATION) {
            sceneDirector->transitionTo(SimulationStage::Complete);
        }

        moonlight->uploadLight(volcanoLightUniforms);

        // Upload simple material properties
        glUniform4f(volcanoUniforms.materialAmbient,
                    volcanoMaterial.Ka.r, volcanoMaterial.Ka.g, volcanoMaterial.Ka.b, volcanoMaterial.Ka.a);
        glUniform4f(volcanoUniforms.materialSpecular,
                    volcanoMaterial.Ks.r, volcanoMaterial.Ks.g, volcanoMaterial.Ks.b, volcanoMaterial.Ks.a);
        glUniform1f(volcanoUniforms.materialShininess, volcanoMaterial.Ns);

        // draw volcano
        volcano->Draw();

        // Domino chain (Part B.4d): actual grass blades grow only after the river
        // is full; the flower geometry is added after the grass is complete.
        if ((grassField && grassGrowth > 0.0f) || !floraProps.empty()) {
            glUseProgram(propShaderProgram);
            glUniformMatrix4fv(propVLoc, 1, GL_FALSE, &viewMatrix[0][0]);
            glUniformMatrix4fv(propPLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
            glUniform3fv(propCameraLocation, 1, &camera->position[0]);
            glUniform1f(propLightningLocation,
                        lightningSystem ? lightningSystem->flashStrength() : 0.0f);
            glUniform1i(propScorchCountLocation,
                        static_cast<GLint>(scorchPositions.size()));
            if (!scorchPositions.empty()) {
                glUniform2fv(propScorchPositionsLocation,
                             static_cast<GLsizei>(scorchPositions.size()),
                             &scorchPositions[0].x);
                glUniform1fv(propScorchRadiiLocation,
                             static_cast<GLsizei>(scorchRadii.size()),
                             &scorchRadii[0]);
                glUniform1fv(propScorchStrengthsLocation,
                             static_cast<GLsizei>(scorchStrengths.size()),
                             &scorchStrengths[0]);
            }
            glUniform1f(propTimeLoc, simulationTime);
            moonlight->uploadLight(propLightUniforms);

            if (grassField && grassGrowth > 0.0f) {
                const mat4 grassModel(1.0f); // blade vertices are already in world space
                glUniform1i(propGrassPassLoc, 1);
                glUniform1i(propInstancedPassLoc, 0);
                glUniform1f(propGrassGrowthLoc, grassGrowth);
                glUniformMatrix4fv(propMLoc, 1, GL_FALSE, &grassModel[0][0]);
                glUniform3f(propColorLoc, 0.20f, 0.52f, 0.095f);
                grassField->bind();
                grassField->draw();
            }

            glUniform1i(propGrassPassLoc, 0);

            // Repeated flower pieces are sent as two instance batches (stems and
            // blooms), reducing the fully grown meadow from 504 prop draw calls
            // to two while preserving the same growth and breeze animation.
            cylinderPropMatrices.clear();
            cylinderPropColors.clear();
            spherePropMatrices.clear();
            spherePropColors.clear();
            for (const auto& prop : floraProps) {
                float age = lavaTime - floraGrowthStartTime - prop.growthDelay;
                float growth = glm::smoothstep(0.0f, prop.growthDuration, age);
                if (growth <= 0.0f) continue;
                glm::vec3 growingPosition = prop.position;
                growingPosition.y = prop.groundY
                                  + (prop.position.y - prop.groundY) * growth;
                growingPosition.x += std::sin(simulationTime * 0.62f + prop.swayPhase)
                                   * prop.swayAmount * growth;
                mat4 propModel = glm::translate(mat4(1.0f), growingPosition)
                               * glm::scale(mat4(1.0f), prop.scale * growth);
                if (prop.useCylinder) {
                    cylinderPropMatrices.push_back(propModel);
                    cylinderPropColors.push_back(prop.color);
                } else {
                    spherePropMatrices.push_back(propModel);
                    spherePropColors.push_back(prop.color);
                }
            }
            glUniform1i(propInstancedPassLoc, 1);
            propCylinderRenderer->draw(cylinderPropMatrices, cylinderPropColors);
            propSphereRenderer->draw(spherePropMatrices, spherePropColors);
            glUniform1i(propInstancedPassLoc, 0);
        }

        // Trees are alpha-cutout cylindrical billboards. Each quad turns only
        // around world Y, so trunks stay vertical while always facing the camera.
        if (!treeProps.empty()) {
            glUseProgram(treeShaderProgram);
            glUniformMatrix4fv(treeVLoc, 1, GL_FALSE, &viewMatrix[0][0]);
            glUniformMatrix4fv(treePLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
            glUniform3fv(treeCameraLoc, 1, &camera->position[0]);
            glUniform1f(treeTimeLoc, simulationTime);
            glUniform1f(treeLightningLoc,
                        lightningSystem ? lightningSystem->flashStrength() : 0.0f);
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(treeTextureLoc, 0);
            particleQuad->bind();

            // Two texture passes keep the renderer simple and avoid rebinding
            // for every tree while supporting both green and flowering species.
            for (int speciesPass = 0; speciesPass < 2; ++speciesPass) {
                const bool floweringPass = speciesPass == 1;
                glBindTexture(GL_TEXTURE_2D,
                              floweringPass ? almondTreeTexture : treeTexture);
                for (const auto& tree : treeProps) {
                    if (tree.flowering != floweringPass) continue;
                    float age = lavaTime - floraGrowthStartTime - tree.growthDelay;
                    float growth = glm::smoothstep(0.0f, TREE_GROW_DURATION, age);
                    if (growth <= 0.0f) continue;
                    glUniform3fv(treeBaseLoc, 1, &tree.basePosition[0]);
                    glUniform2fv(treeSizeLoc, 1, &tree.size[0]);
                    glUniform3fv(treeTintLoc, 1, &tree.tint[0]);
                    glUniform1f(treeGrowthLoc, growth);
                    glUniform1f(treeSwayLoc, tree.swayPhase);
                    particleQuad->draw();
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Draw smoke/ash/cloud last (transparent)
        bool drawSmoke = smokeEmitter && sceneDirector && lavaTimeForSmoke >= 0.0f;
        if (drawSmoke || calmSmokeEmitter || ambientCloudEmitter || ashEmitter ||
            cloudEmitter || rainEmitter) {
            glDepthMask(GL_FALSE);

            glUseProgram(particleShaderProgram);
            mat4 PV = projectionMatrix * viewMatrix;
            glUniformMatrix4fv(particlePVLocation, 1, GL_FALSE, &PV[0][0]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, smokeTexture);
            glUniform1i(particleTextureSampler, 0);

            // Each emitter shares the smoke texture but gets its own tint + opacity so
            // the four stages read distinctly: grey volcano smoke, dark ash, pale cloud,
            // bright blue rain.
            if (drawSmoke) {
                glUniform3f(particleTintLocation, 0.255f, 0.25f, 0.26f);
                const float heavySmokeFade = 1.0f
                    - glm::smoothstep(0.0f, 0.58f, calmProgress);
                glUniform1f(particleAlphaLocation, 0.27f * heavySmokeFade);
                glUniform2f(particleShapeScaleLocation, 1.10f, 1.0f);
                glUniform1f(particlePuffinessLocation, 1.0f);
                glUniform1f(particleShearLocation, 0.0f);
                smokeEmitter->renderParticles();
            }
            if (calmSmokeEmitter) {
                const float steamFormation = glm::smoothstep(0.06f, 0.82f,
                                                              calmProgress);
                glUniform3f(particleTintLocation, 0.84f, 0.85f, 0.88f);
                glUniform1f(particleAlphaLocation, 0.15f * steamFormation);
                glUniform2f(particleShapeScaleLocation, 0.82f, 0.94f);
                glUniform1f(particlePuffinessLocation, 0.94f);
                glUniform1f(particleShearLocation, 0.0f);
                calmSmokeEmitter->renderParticles();
            }
            if (ambientCloudEmitter) {
                glUniform3f(particleTintLocation, 0.41f, 0.43f, 0.50f);
                glUniform1f(particleAlphaLocation,
                            glm::mix(0.30f, 0.035f, calmProgress));
                glUniform2f(particleShapeScaleLocation, 1.35f, 0.72f);
                glUniform1f(particlePuffinessLocation, 0.9f);
                glUniform1f(particleShearLocation, 0.0f);
                ambientCloudEmitter->renderParticles();
            }
            if (ashEmitter) {
                float ashAge = sceneDirector &&
                               sceneDirector->getStage() == SimulationStage::SmokeAndAsh
                    ? sceneDirector->getStageElapsedSeconds()
                    : CLOUD_FORM_DELAY;
                float ashFormation = glm::smoothstep(0.0f, 1.4f, ashAge);
                float ashCooling = glm::smoothstep(0.0f, LAVA_COOL_DURATION, ashAge);
                float ashFade = cloudEmitter
                    ? 1.0f - glm::smoothstep(0.0f, 6.0f,
                                             lavaTime - (lavaCoolStartTime + CLOUD_FORM_DELAY))
                    : 1.0f;
                glm::vec3 warmAsh(0.22f, 0.185f, 0.17f);
                glm::vec3 coolAsh(0.145f, 0.15f, 0.17f);
                glm::vec3 ashTint = glm::mix(warmAsh, coolAsh, ashCooling);
                glUniform3f(particleTintLocation, ashTint.r, ashTint.g, ashTint.b);
                glUniform1f(particleAlphaLocation, 0.24f * ashFormation * ashFade);
                glUniform2f(particleShapeScaleLocation, 0.78f, 1.08f);
                glUniform1f(particlePuffinessLocation, 0.92f);
                glUniform1f(particleShearLocation, 0.0f);
                ashEmitter->renderParticles();
            }
            if (cloudEmitter) {
                glUniform3f(particleTintLocation, 0.37f, 0.39f, 0.47f);
                glUniform1f(particleAlphaLocation,
                            0.42f * (1.0f - calmProgress));
                glUniform2f(particleShapeScaleLocation, 1.28f, 0.68f);
                glUniform1f(particlePuffinessLocation, 0.95f);
                glUniform1f(particleShearLocation, 0.0f);
                cloudEmitter->renderParticles();
            }
            if (rainEmitter) {
                float rainFade = floraSpawned
                    ? 1.0f - glm::smoothstep(0.0f, 5.0f, lavaTime - floraGrowthStartTime)
                    : 1.0f;
                float rainAge = glm::max(0.0f, lavaTime - rainStartTime);
                float rainBuild = glm::smoothstep(0.0f, 1.6f, rainAge);
                glUniform3f(particleTintLocation, 0.58f, 0.72f, 0.90f);
                glUniform1f(particleAlphaLocation, 0.82f * rainBuild * rainFade);
                glUniform2f(particleShapeScaleLocation, 0.17f, 6.2f);
                glUniform1f(particlePuffinessLocation, 0.0f);
                glUniform1f(particleShearLocation, 0.16f);
                rainEmitter->renderParticles();
            }
            // Restore defaults for any later particle draws.
            glUniform3f(particleTintLocation, 1.0f, 1.0f, 1.0f);
            glUniform1f(particleAlphaLocation, 1.0f);
            glUniform2f(particleShapeScaleLocation, 1.0f, 1.0f);
            glUniform1f(particlePuffinessLocation, 0.0f);
            glUniform1f(particleShearLocation, 0.0f);

            glBindTexture(GL_TEXTURE_2D, 0);
            glDepthMask(GL_TRUE);
        }

        if (lightningSystem) {
            glDepthMask(GL_FALSE);
            lightningSystem->draw(viewMatrix, projectionMatrix);
            glDepthMask(GL_TRUE);
        }

        #if defined(ELEMENTAL_ENABLE_IMGUI)
        if (gShowHud) {
            ImGui::SetNextWindowBgAlpha(0.72f);
            ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(340.0f, 0.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Elemental controls", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            if (sceneDirector) {
                ImGui::Text("Stage: %s", sceneDirector->getStageName());
                ImGui::SameLine();
                ImGui::TextDisabled("  %.1fs", sceneDirector->getSimSeconds());

                float selectedScale = sceneDirector->getTimeScale();
                if (ImGui::SliderFloat("Simulation speed", &selectedScale, 0.0f, 8.0f, "%.2fx")) {
                    sceneDirector->setTimeScale(selectedScale);
                }
                if (ImGui::Button(sceneDirector->isPaused() ? "Resume" : "Pause")) {
                    sceneDirector->setPaused(!sceneDirector->isPaused());
                }
                ImGui::SameLine();
                if (ImGui::Button("Normal speed")) sceneDirector->setTimeScale(1.0f);
                ImGui::SameLine();
                if (ImGui::Button("Restart")) resetSimulation(currentTime);

                if (ImGui::Button("Lightning")) manualLightningRequested = true;
                ImGui::SameLine();
                if (ImGui::Button("Reset camera")) camera->resetToEstablishingShot();

                ImGui::ProgressBar(waterFill, ImVec2(-1.0f, 0.0f), "River water");
                ImGui::Separator();
                ImGui::TextDisabled("F1 close  |  C camera  |  P pause  |  [ ] speed  |  R restart");
            }
            ImGui::End();
        }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #endif

        glDepthMask(GL_TRUE);

        if (firstFrameDiagnostics) {
            throwOnOpenGLError("first complete frame");
            firstFrameDiagnostics = false;
        }

        if (!gScreenshotCaptured && !gScreenshotPath.empty() && sceneDirector &&
            sceneDirector->getStage() == gScreenshotStage &&
            sceneDirector->getStageElapsedSeconds() >= gScreenshotStageDelay) {
            glReadBuffer(GL_BACK);
            gScreenshotCaptured = SOIL_save_screenshot(
                gScreenshotPath.c_str(), SOIL_SAVE_TYPE_BMP, 0, 0,
                framebufferWidth, framebufferHeight) != 0;
            std::cout << (gScreenshotCaptured ? "Saved screenshot: " : "Failed to save screenshot: ")
                      << gScreenshotPath << std::endl;
        }

        glfwSwapBuffers(window);
        ++renderedFrameCount;

        glfwPollEvents();

        const bool screenshotRunFinished = !gScreenshotPath.empty() && gScreenshotCaptured;
        const bool simulationRunFinished = gScreenshotPath.empty() && sceneDirector &&
            sceneDirector->getStage() == SimulationStage::Complete;
        if (gAutoExitOnComplete && (screenshotRunFinished || simulationRunFinished)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);

    if (gReportPerformance) {
        const double measuredSeconds = glfwGetTime() - performanceStartTime;
        const double averageFps = measuredSeconds > 0.0
            ? static_cast<double>(renderedFrameCount) / measuredSeconds
            : 0.0;
        std::cout << std::fixed << std::setprecision(1)
                  << "Performance: " << renderedFrameCount << " frames in "
                  << measuredSeconds << " s (" << averageFps
                  << " average FPS)" << std::endl;
        if (!frameTimesMs.empty()) {
            std::sort(frameTimesMs.begin(), frameTimesMs.end());
            const auto percentile = [&](float fraction) {
                const std::size_t index = static_cast<std::size_t>(
                    fraction * static_cast<float>(frameTimesMs.size() - 1));
                return frameTimesMs[index];
            };
            const std::size_t framesOver16Ms = static_cast<std::size_t>(std::count_if(
                frameTimesMs.begin(), frameTimesMs.end(),
                [](float milliseconds) { return milliseconds > 16.67f; }));
            const std::size_t framesOver33Ms = static_cast<std::size_t>(std::count_if(
                frameTimesMs.begin(), frameTimesMs.end(),
                [](float milliseconds) { return milliseconds > 33.33f; }));
            std::cout << "Frame time: p95 " << percentile(0.95f)
                      << " ms, p99 " << percentile(0.99f)
                      << " ms, max " << frameTimesMs.back() << " ms; "
                      << framesOver16Ms << " frames >16.7 ms, "
                      << framesOver33Ms << " frames >33.3 ms" << std::endl;
        }
    }
}

void initialize()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        throw runtime_error("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE,
                              gWindowed ? NULL : glfwGetPrimaryMonitor(), NULL);
    if (window == NULL)
    {
        throw runtime_error(string(string("Failed to open GLFW window.") +
            " If you have an Intel GPU, they are not 3.3 compatible." +
            "Try the 2.1 version.\n"));
    }
    glfwMakeContextCurrent(window);
    // Interactive presentation runs are capped to the display refresh rate.
    // Automated performance/screenshot runs remain uncapped and finish quickly.
    glfwSwapInterval((gReportPerformance || gAutoExitOnComplete ||
                      !gScreenshotPath.empty()) ? 0 : 1);

    // Start GLEW extension handler
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        throw runtime_error("Failed to initialize GLEW\n");
    }

    // GLEW 1.x probes legacy entry points while starting a core-profile context
    // and can leave a benign GL_INVALID_ENUM behind. Clear only this startup
    // residue so all later diagnostic checkpoints begin from a clean state.
    while (glGetError() != GL_NO_ERROR) {}
    gOpenGLContextReady = true;

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

    // Gray background color
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);
    if (!gScreenshotPath.empty()) {
        // Automated captures run without window focus, so do not interpret the
        // desktop pointer position as an intentional camera movement.
        camera->setMouseLookEnabled(false);
    }

    moonlight = new Light(window,
        vec4{0.040, 0.040, 0.200, 1.0},
        vec4{0.34, 0.50, 0.76, 1.0},
        vec4{0.68, 0.77, 0.90, 1.0},
        vec3{0, 300, 350},
        // Aim at the volcano center (must match the Volcano ctor's volcanoCenter in createContext()).
        vec3{ 0, 0, -400 },
        // Ortho half-extent large enough to cover the ~1350-wide terrain, near/far covering
        // the light-to-terrain distance (~800 units).
        900.0f, 50.0f, 1400.0f
    );

#if defined(ELEMENTAL_ENABLE_IMGUI)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        throw runtime_error("Failed to initialize the ImGui GLFW backend");
    }
    gImGuiGlfwInitialized = true;
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        throw runtime_error("Failed to initialize the ImGui OpenGL backend");
    }
    gImGuiOpenGLInitialized = true;
#endif

    throwOnOpenGLError("OpenGL initialization");
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--windowed") {
            gWindowed = true;
        } else if (argument == "--auto-exit") {
            gAutoExitOnComplete = true;
        } else if (argument == "--report-performance") {
            gReportPerformance = true;
        } else if (argument == "--screenshot" && i + 1 < argc) {
            gScreenshotPath = argv[++i];
        } else if (argument == "--screenshot-stage" && i + 1 < argc) {
            const std::string stage(argv[++i]);
            if (stage == "awakening") gScreenshotStage = SimulationStage::Awakening;
            else if (stage == "lava") gScreenshotStage = SimulationStage::LavaFlowing;
            else if (stage == "ash") gScreenshotStage = SimulationStage::SmokeAndAsh;
            else if (stage == "cloud") gScreenshotStage = SimulationStage::CloudFormation;
            else if (stage == "rain") gScreenshotStage = SimulationStage::Raining;
            else if (stage == "river") gScreenshotStage = SimulationStage::RiverFilling;
            else if (stage == "vegetation") gScreenshotStage = SimulationStage::VegetationGrowing;
            else if (stage == "storm") gScreenshotStage = SimulationStage::LightningStorm;
            else if (stage == "electric") gScreenshotStage = SimulationStage::ElectrifiedRiver;
            else if (stage == "calm") gScreenshotStage = SimulationStage::CalmNight;
            else gScreenshotStage = SimulationStage::Complete;
        } else if (argument == "--screenshot-delay" && i + 1 < argc) {
            gScreenshotStageDelay = glm::max(0.0f, static_cast<float>(std::atof(argv[++i])));
        } else if (argument == "--speed" && i + 1 < argc) {
            gInitialTimeScale = glm::clamp(static_cast<float>(std::atof(argv[++i])), 0.0f, 8.0f);
        }
    }

    try
    {
        initialize();
        createContext();
        mainLoop();
        free();
    }
    catch (exception& ex)
    {
        cerr << ex.what() << endl;
        free();
        return -1;
    }

    return 0;
}

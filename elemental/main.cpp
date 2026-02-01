// Include C++ headers
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

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
#include <elemental/ParticleSystem.h>
#include <elemental/ParticleShaderParams.h>
#include <common/light.h>

// Crack system (CPU -> shader + optional stone burst)
#include <elemental/CrackSystem.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "ELEMENTAL"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint volcanoShaderProgram, particleShaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint terrainTextureSampler, smokeTextureSampler;
GLuint terrainTexture, smokeTexture;

ParticleSystem* smokeSystem;
ParticleSystem* sparksSystem;

// Cache particle shader uniform locations + params (so we don't glGetUniformLocation every frame)
static ParticleShaderUniformLocations gParticleU;
static ParticleShaderParams gSmokeShaderParams;
static ParticleShaderParams gSparksShaderParams;

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

// ------------------------------
// Crack events (CPU -> shader)
// ------------------------------
static CrackSystem gCrackSystem;
static bool gCracksCreated = false;

void createContext()
{
    // Load shaders
    volcanoShaderProgram = loadShaders("../elemental/shaders/Volcano.vertexshader", "../elemental/shaders/Volcano.fragmentshader");
    particleShaderProgram = loadShaders("../elemental/shaders/Particle.vertexshader", "../elemental/shaders/Particle.fragmentshader");

    // Get uniform locations for main shader
    projectionMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "M");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Create volcano
    volcano = new Volcano(512, 1350.0f, 20.0f, glm::vec2(0.0f, -400.0f));

    // Load terrain texture
    terrainTexture = loadSOIL("/Users/nikos/Desktop/elemental/elemental/assets/Diffusemap.png");
    

    terrainTextureSampler = glGetUniformLocation(volcanoShaderProgram, "uTerrainTexture");
    

    glBindTexture(GL_TEXTURE_2D, terrainTexture);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D); 

    glBindTexture(GL_TEXTURE_2D, 0);

    smokeTexture = loadSOIL("/Users/nikos/Desktop/elemental/elemental/assets/smoke3.png");
    smokeTextureSampler = glGetUniformLocation(particleShaderProgram, "uSmokeTexture");

    // Cache particle shader uniform locations once
    gParticleU = getParticleShaderUniformLocations(particleShaderProgram);

    // Smoke defaults (match previous hardcoded values)
    gSmokeShaderParams = ParticleShaderParams{};

    // Sparks look (reusing same shader)
    gSparksShaderParams = ParticleShaderParams{};
    gSparksShaderParams.sizeScale = 0.95f;
    gSparksShaderParams.wobbleAmp = 0.0f;
    gSparksShaderParams.featherEdges = glm::vec2(1.0f, 0.10f);
    gSparksShaderParams.densityEdges = glm::vec2(0.05f, 0.90f);
    gSparksShaderParams.alphaDiscard = 0.008f;
    gSparksShaderParams.rimIntensity = 0.0f;
    gSparksShaderParams.texRgbMix = 0.0f;
    gSparksShaderParams.densityMulAdd = glm::vec2(0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, smokeTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // skybox
    std::vector<std::string> cubemapFaces = {
        "../skybox_blue/right.png",
        "../skybox_blue/left.png",
        "../skybox_blue/top.png",
        "../skybox_blue/bottom.png",
        "../skybox_blue/front.png",
        "../skybox_blue/back.png"
    };
    skybox = new Skybox(cubemapFaces);

    sceneDirector = new SceneDirector(camera, moonlight, volcano, skybox);

    smokeSystem = new ParticleSystem(2500);
    sparksSystem = new ParticleSystem(200);

    stats = volcano->getStats();

    // Smoke config
    ParticleEffectConfig smokeCfg;
    smokeCfg.emitterRadius = stats.craterRadius * 0.95f;
    smokeSystem->SetConfig(smokeCfg);

    // Sparks config (uses the same simulation code, tuned to look like embers)
    ParticleEffectConfig sparksCfg;
    sparksCfg.spawnRate     = 20.0f;
    sparksCfg.lifetime      = 10.5f;
    sparksCfg.emitterRadius = stats.craterRadius * 0.75f;

    sparksCfg.startColor    = glm::vec4(1.0f, 0.65f, 0.15f, 0.85f);

    sparksCfg.startSizeMin  = 1.5f;
    sparksCfg.startSizeMax  = 5.65f;
    sparksCfg.endSizeMul    = 0.35f; // shrink over life

    sparksCfg.upSpeedMin    = 18.0f;
    sparksCfg.upSpeedMax    = 35.0f;

    sparksCfg.spreadMin     = 0.2f;
    sparksCfg.spreadMax     = 1.1f;

    sparksCfg.dragPerSec    = 0.98f;
    sparksCfg.wind          = glm::vec3(0.2f, 0.0f, 0.15f);

    sparksSystem->SetConfig(sparksCfg);
}

void free()
{
    glDeleteProgram(volcanoShaderProgram);

    // Clean up allocated objects
    if (volcano) delete volcano;
    if (skybox) delete skybox;
    if (sceneDirector) delete sceneDirector;
    if (terrainTexture) {
        glDeleteTextures(1, &terrainTexture);
        terrainTexture = 0;
    }
    if (smokeSystem) delete smokeSystem;
    if (sparksSystem) delete sparksSystem;
    if (moonlight) delete moonlight;

    glfwTerminate();
}

void mainLoop()
{
    double lastTime = glfwGetTime();

    do
    {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        if (sceneDirector) {
            sceneDirector->update(currentTime, deltaTime);
        } else if (camera) {
            camera->update();
        }
        glm::vec3 emitter(stats.craterCenter.x,
                  (stats.craterTop - stats.craterBottom)/2 + 5.0f,   // tune: 3..10
                  stats.craterCenter.y);

        // Spawn sparks from slightly lower (inside crater) than smoke
        glm::vec3 sparksEmitter = emitter + glm::vec3(0.0f, 87.0f, 0.0f);

        smokeSystem->Update(deltaTime, emitter);
        sparksSystem->Update(deltaTime, sparksEmitter);

        // Update light
        moonlight->update();
        
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        mat4 modelMatrix = mat4(1.0);

        // Draw skybox first
        if (skybox)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            skybox->Draw(viewMatrix, projectionMatrix);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glUseProgram(volcanoShaderProgram);

        // transfer uniforms to GPU
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        // Bind terrain texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrainTexture);
        glUniform1i(terrainTextureSampler, 0);
        
        // Send time uniform
        float lavaTime = sceneDirector ? sceneDirector->getLavaTimeSeconds()
                                       : static_cast<float>(currentTime);
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "u_LavaTime"), lavaTime);

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
                gCrackSystem.addCrack(stats, p, lavaTime + 0.30f, 1.4f, 80.0f, radialDir(p), 130.0f);
            }
            {
                glm::vec2 p(320.0f, -240.0f);
                gCrackSystem.addCrack(stats, p, lavaTime + 0.00f, 1.9f, 95.0f, radialDir(p), 160.0f);
            }
            {
                glm::vec2 p(-200.0f, -340.0f);
                gCrackSystem.addCrack(stats, p, lavaTime + 0.15f, 1.6f, 85.0f, radialDir(p), 140.0f);
            }
            {
                glm::vec2 p(-170.0f, -180.0f);
                gCrackSystem.addCrack(stats, p, lavaTime + 0.45f, 1.7f, 90.0f, radialDir(p), 150.0f);
            }
            {
                glm::vec2 p(180.0f, -60.0f);
                gCrackSystem.addCrack(stats, p, lavaTime + 0.60f, 1.1f, 75.0f, radialDir(p), 120.0f);
            }

            gCracksCreated = true;
        }

        // Upload crack uniforms to the volcano shader (cheap, small arrays)
        gCrackSystem.uploadToVolcanoShader(volcanoShaderProgram);

        VolcanoStats stats = volcano->getStats();
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "u_CraterRadius"), stats.craterRadius);
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "u_CraterBottom"), stats.craterBottom);
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "u_CraterTop"), stats.craterTop);
        
        moonlight->uploadLight(volcanoShaderProgram, 0);

        // Upload simple material properties
        glUniform4f(glGetUniformLocation(volcanoShaderProgram, "Ka"), 
                    volcanoMaterial.Ka.r, volcanoMaterial.Ka.g, volcanoMaterial.Ka.b, volcanoMaterial.Ka.a);
        glUniform4f(glGetUniformLocation(volcanoShaderProgram, "Ks"), 
                    volcanoMaterial.Ks.r, volcanoMaterial.Ks.g, volcanoMaterial.Ks.b, volcanoMaterial.Ks.a);
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "Ns"), volcanoMaterial.Ns);

        // draw volcano
        volcano->Draw();

        // Draw particles (smoke)
        glUseProgram(particleShaderProgram);

        applyParticleShaderParams(
            gParticleU,
            projectionMatrix,
            viewMatrix,
            glm::vec3(0.7f, 0.8f, 1.0f),
            (float)currentTime,
            gSmokeShaderParams
        );

        // texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smokeTexture);
        if (gParticleU.smokeTexture != -1) glUniform1i(gParticleU.smokeTexture, 0);

        // blending/depth for smoke
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        smokeSystem->Draw(particleShaderProgram);

        // Draw particles (sparks)
        applyParticleShaderParams(
            gParticleU,
            projectionMatrix,
            viewMatrix,
            glm::vec3(0.0f), // no moon rim contribution for sparks
            (float)currentTime,
            gSparksShaderParams
        );

        // keep same texture as a mask
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smokeTexture);
        if (gParticleU.smokeTexture != -1) glUniform1i(gParticleU.smokeTexture, 0);

        // additive blending for sparks
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        sparksSystem->Draw(particleShaderProgram);

        // IMPORTANT: restore default blending state so the rest of the frame doesn't get brightened
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthMask(GL_TRUE);

        glfwSwapBuffers(window);

        glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);
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
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, glfwGetPrimaryMonitor(), NULL);
    if (window == NULL)
    {
        glfwTerminate();
        throw runtime_error(string(string("Failed to open GLFW window.") +
            " If you have an Intel GPU, they are not 3.3 compatible." +
            "Try the 2.1 version.\n"));
    }
    glfwMakeContextCurrent(window);

    // Start GLEW extension handler
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw runtime_error("Failed to initialize GLEW\n");
    }

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

    glEnable(GL_TEXTURE_2D);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);

    moonlight = new Light(window,
        vec4{0.05, 0.05, 0.25, 1.0},
        vec4{0.4, 0.6, 0.9, 1.0},
        vec4{0.8, 0.9, 1.0, 1.0},
        vec3{ 0, 300, 350 }
    );
}

int main(void)
{
    try
    {
        initialize();
        createContext();
        mainLoop();
        free();
    }
    catch (exception& ex)
    {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}

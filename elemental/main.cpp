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
#include <common/light.h>
#include <elemental/elements/SmokeEmitter.h>

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

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "ELEMENTAL"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint volcanoShaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint terrainTextureSampler;
GLuint terrainTexture;

// Particle shader + smoke resources
GLuint particleShaderProgram;
GLuint particlePVLocation;
GLuint particleTextureSampler;
GLuint smokeTexture;
Drawable* smokeQuad;
SmokeEmitter* smokeEmitter;

#if defined(ELEMENTAL_ENABLE_IMGUI)
static bool gShowImGuiDemo = false;
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

// ------------------------------
// Crack events (CPU -> shader)
// ------------------------------
static CrackSystem gCrackSystem;
static bool gCracksCreated = false;

void createContext()
{
    // Load shaders
    volcanoShaderProgram = loadShaders("../elemental/shaders/Volcano.vertexshader", "../elemental/shaders/Volcano.fragmentshader");
    particleShaderProgram = loadShaders("../elemental/shaders/ParticleShader.vertexshader", "../elemental/shaders/ParticleShader.fragmentshader");

    // Get uniform locations for main shader
    projectionMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(volcanoShaderProgram, "M");

    // Particle shader uniforms
    particlePVLocation = glGetUniformLocation(particleShaderProgram, "PV");
    particleTextureSampler = glGetUniformLocation(particleShaderProgram, "texture0");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Create volcano
    volcano = new Volcano(512, 1350.0f, 20.0f, glm::vec2(0.0f, -400.0f));
    stats = volcano->getStats();

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

    // Smoke: spawn at crater and start emitting after the shake window ends (lava starts)
    smokeQuad = new Drawable("../elemental/assets/sphere1.obj");
    smokeTexture = loadSOIL("../elemental/assets/smoke3.png");
    glm::vec3 craterPos(stats.craterCenter.x, stats.craterTop - 132.0f, stats.craterCenter.y);
    smokeEmitter = new SmokeEmitter(smokeQuad, 10000, craterPos);
}

void free()
{
    glDeleteProgram(volcanoShaderProgram);
    glDeleteProgram(particleShaderProgram);

    // Clean up allocated objects
    if (volcano) delete volcano;
    if (skybox) delete skybox;
    if (sceneDirector) delete sceneDirector;
    if (smokeEmitter) delete smokeEmitter;
    if (smokeQuad) delete smokeQuad;
    if (smokeTexture) {
        glDeleteTextures(1, &smokeTexture);
        smokeTexture = 0;
    }
    if (terrainTexture) {
        glDeleteTextures(1, &terrainTexture);
        terrainTexture = 0;
    }
    if (moonlight) delete moonlight;

#if defined(ELEMENTAL_ENABLE_IMGUI)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif

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

#if defined(ELEMENTAL_ENABLE_IMGUI)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif

        if (sceneDirector) {
            sceneDirector->update(currentTime, deltaTime);
        } else if (camera) {
            camera->update();
        }

        // Smoke should start after camera trembling ends.
        // SceneDirector defines lavaTimeSeconds >= 0 at m_lavaStartSeconds (end of shake).
        float lavaTimeForSmoke = sceneDirector ? sceneDirector->getLavaTimeSeconds()
                                               : static_cast<float>(currentTime);
        if (smokeEmitter && sceneDirector && lavaTimeForSmoke >= 0.0f) {
            smokeEmitter->updateParticles(static_cast<float>(currentTime), deltaTime, camera->position);
        }

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

        // Draw smoke last (transparent)
        if (smokeEmitter && sceneDirector && lavaTimeForSmoke >= 0.0f) {
            glDepthMask(GL_FALSE);

            glUseProgram(particleShaderProgram);
            mat4 PV = projectionMatrix * viewMatrix;
            glUniformMatrix4fv(particlePVLocation, 1, GL_FALSE, &PV[0][0]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, smokeTexture);
            glUniform1i(particleTextureSampler, 0);

            smokeEmitter->renderParticles();

            glBindTexture(GL_TEXTURE_2D, 0);
            glDepthMask(GL_TRUE);
        }

        #if defined(ELEMENTAL_ENABLE_IMGUI)
            ImGui::Begin("Elemental");
            ImGui::Checkbox("Show ImGui Demo", &gShowImGuiDemo);
            ImGui::Text("Lava time: %.2f", lavaTime);
            ImGui::Text("Camera pos: (%.1f, %.1f, %.1f)", camera->position.x, camera->position.y, camera->position.z);





            ImGui::End();
            if (gShowImGuiDemo) ImGui::ShowDemoWindow(&gShowImGuiDemo);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #endif

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

#if defined(ELEMENTAL_ENABLE_IMGUI)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
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

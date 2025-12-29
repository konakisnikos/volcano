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
#include <common/light.h>

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

Light* moonlight;

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

void createContext()
{
    // Load shaders
    volcanoShaderProgram = loadShaders("../elemental/shaders/Volcano.vertexshader", "../elemental/shaders/Volcano.fragmentshader");

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
}

void free()
{
    glDeleteProgram(volcanoShaderProgram);

    // Clean up allocated objects
    if (volcano) delete volcano;
    if (skybox) delete skybox;
    if (terrainTexture) {
        glDeleteTextures(1, &terrainTexture);
        terrainTexture = 0;
    }

    glfwTerminate();
}

void mainLoop()
{
    double lastTime = glfwGetTime();

    do
    {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime)/2;
        lastTime = currentTime;

        // Update light
        moonlight->update();
        
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camera
        camera->update();

        cout << "Camera position: "
             << camera->position.x << ", "
             << camera->position.y << ", "
             << camera->position.z << " \r";
        cout.flush();

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        mat4 modelMatrix = mat4(1.0);

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
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "u_Time"), currentTime);

        moonlight->uploadLight(volcanoShaderProgram, 0);

        // Upload simple material properties
        glUniform4f(glGetUniformLocation(volcanoShaderProgram, "Ka"), 
                    volcanoMaterial.Ka.r, volcanoMaterial.Ka.g, volcanoMaterial.Ka.b, volcanoMaterial.Ka.a);
        glUniform4f(glGetUniformLocation(volcanoShaderProgram, "Ks"), 
                    volcanoMaterial.Ks.r, volcanoMaterial.Ks.g, volcanoMaterial.Ks.b, volcanoMaterial.Ks.a);
        glUniform1f(glGetUniformLocation(volcanoShaderProgram, "Ns"), volcanoMaterial.Ns);

        // draw volcano
        volcano->Draw();

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
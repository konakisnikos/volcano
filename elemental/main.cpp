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
#include <common/Volcano.h>
#include <common/Skybox.h>

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
GLuint shaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;

GLint terrainTextureSampler;

// Removed unused lightLocation for simplified shaders
// GLuint lightLocation;
Volcano* volcano = nullptr;
Skybox* skybox = nullptr;
GLuint terrainTexture = 0;


void createContext()
{
    // Create and compile our GLSL program from the shaders
    shaderProgram = loadShaders("../elemental/Volcano.vertexshader", "../elemental/Volcano.fragmentshader");

    // get pointers to the uniform variables
    projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    // lightLocation no longer needed
    // lightLocation = glGetUniformLocation(shaderProgram, "light_position_worldspace");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // volcano
    // Create procedural volcano with grid size 194, terrain width 950.0, height scale 17.0
    volcano = new Volcano(194, 950.0f, 17.0f);

    //terrainTexture = loadSOIL("../elemental/Diffusemap.png");

    //terrainTextureSampler = glGetUniformLocation(shaderProgram, "uTerrainTexture");

    // skybox

    std::vector<std::string> cubemapFaces = {
        "../skybox/right.png",
        "../skybox/left.png",
        "../skybox/top.png",
        "../skybox/bottom.png",
        "../skybox/front.png",
        "../skybox/back.png"
    };
    skybox = new Skybox(cubemapFaces);
}

void free()
{
    glDeleteProgram(shaderProgram);

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
    // glm::vec3 lightPos = glm::vec3(0, 0, 4); // no lighting for now

    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camera
        camera->update();

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        mat4 modelMatrix = mat4(1.0);

        if (skybox)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            skybox->Draw(viewMatrix, projectionMatrix);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glUseProgram(shaderProgram);

        // transfer uniforms to GPU
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        // glUniform3f(lightLocation, lightPos.x, lightPos.y, lightPos.z); // light disabled

        
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, terrainTexture);
        //glUniform1i(terrainTextureSampler, 0);
        

        

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
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
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
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);
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

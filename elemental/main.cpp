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

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Lab 05"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint lightLocation;
GLuint diffuseColorSampler, specularColorSampler;
GLuint diffuseTexture, specularTexture;
std::vector<vec3> objVertices, objNormals;
std::vector<vec2> objUVs;
Drawable* triangle;
Drawable* obj;

//#define RENDER_TRIANGLE

void createContext()
{
    // Create and compile our GLSL program from the shaders
    shaderProgram = loadShaders("PhongShading.vertexshader", "PhongShading.fragmentshader");

    // Homework 2: implement Gouraud shading.
    // shaderProgram = loadShaders("GouraudShading.vertexshader", "GouraudShading.fragmentshader");

    // Homework 3: implement flat shading.
    // shaderProgram = loadShaders("FlatShading.vertexshader", "FlatShading.fragmentshader");
    

    // Task 6.2: load diffuse and specular texture maps
    diffuseTexture = loadSOIL("suzanne_diffuse.bmp");
    specularTexture = loadSOIL("suzanne_specular.bmp");


    // Task 6.3: get a pointer to the texture samplers (diffuseColorSampler, specularColorSampler)
    diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
    specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");


    // get pointers to the uniform variables
    projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    lightLocation = glGetUniformLocation(shaderProgram, "light_position_worldspace");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // triangle
    // Task 1: visualize a triangle facing towards the +z direction.
    // Task 1.1: define the triangle’s vertex positions
    vector<vec3> triangleVertices = {
        vec3(-1.5, -1.5, 0.0),
        vec3(0.0, 1.5, 0.0),
        vec3(1.5, -1.5, 0.0)
    };

    //triangleVertices.push_back(vec3(...)); // vertex 1
    

    // Task 1.2: define the triangle’s vertex normals
    vector<vec3> triangleNormals = {
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 0.0, 1.0)
    };


    // Task 1.3: construct the triangle as a Drawable
    triangle = new Drawable(triangleVertices, VEC_VEC2_DEFAULT_VALUE, triangleNormals);


    // obj
    // Task 1.4: construct the object (Suzanne) as a Drawable
    obj = new Drawable("suzanne.obj");


    // Homework 7: Load the obj's vertices and UVs using loadOBJWithTiny.
    // Ignore the normals returned by loadOBJWithTiny and try to compute them yourself.
    // loadOBJWithTiny("suzanne.obj", objVertices, objUVs, objNormals);
    // loadOBJWithTiny("earth.obj", objVertices, objUVs, objNormals);

}

void free()
{
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void mainLoop()
{
    glm::vec3 lightPos = glm::vec3(0, 0, 4);

    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // camera
        camera->update();

        // Task 1.5: bind
#ifdef RENDER_TRIANGLE
        // bind triangle
        triangle->bind();

#else
        // bind obj
        obj->bind();

#endif
        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        mat4 modelMatrix = mat4(1.0);

        // transfer uniforms to GPU
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniform3f(lightLocation, lightPos.x, lightPos.y, lightPos.z); // light

        // Task 6.4: bind textures and transmit the diffuse and specular maps to the GPU
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        glUniform1i(diffuseColorSampler, 0); // Texture unit 0

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTexture);
        glUniform1i(specularColorSampler, 1); // Texture unit 1

        // Task 1.5: draw
#ifdef RENDER_TRIANGLE
        // draw triangle
        triangle->draw();

#else
        // draw obj
        obj->draw();

#endif

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
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
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

    // enable textures
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

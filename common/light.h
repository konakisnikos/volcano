#include <glm/glm.hpp>
#include <GL/glew.h>

struct LightUniformLocations {
    GLint ambient = -1;
    GLint diffuse = -1;
    GLint specular = -1;
    GLint position = -1;
    GLint direction = -1;
};

class Light {
public:

    GLFWwindow* window;
    // Light parameters
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    glm::vec3 lightPosition_worldspace;

    glm::vec4 La;
    glm::vec4 Ld;
    glm::vec4 Ls;

    float nearPlane;
    float farPlane;

    float lightSpeed;
    glm::vec3 direction;

    // Where the light will look at
    glm::vec3 targetPosition;

    // Constructor
    // orthoHalfExtent/init_nearPlane/init_farPlane size the light's ortho
    // shadow frustum; they must be large enough to cover the terrain as
    // seen from init_position looking at init_target.
    Light(GLFWwindow* window,
        glm::vec4 init_La,
        glm::vec4 init_Ld,
        glm::vec4 init_Ls,
        glm::vec3 init_position,
        glm::vec3 init_target = glm::vec3(0.0f, 0.0f, -5.0f),
        float orthoHalfExtent = 10.0f,
        float init_nearPlane = 1.0f,
        float init_farPlane = 30.0f);

    void update();

    // Uniform locations are resolved once after linking, then reused every frame.
    static LightUniformLocations findUniformLocations(GLuint shaderProgram,
                                                      int lightIndex = 0);
    void uploadLight(const LightUniformLocations& locations) const;

    glm::mat4 lightVP();
};

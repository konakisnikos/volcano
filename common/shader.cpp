#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
using namespace std;

#include "shader.h"

namespace {

std::string shaderInfoLog(GLuint shaderID) {
    GLint infoLogLength = 0;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength <= 1) return "";

    std::vector<char> log(static_cast<size_t>(infoLogLength), '\0');
    glGetShaderInfoLog(shaderID, infoLogLength, NULL, &log[0]);
    return std::string(&log[0]);
}

std::string programInfoLog(GLuint programID) {
    GLint infoLogLength = 0;
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength <= 1) return "";

    std::vector<char> log(static_cast<size_t>(infoLogLength), '\0');
    glGetProgramInfoLog(programID, infoLogLength, NULL, &log[0]);
    return std::string(&log[0]);
}

void deleteShader(GLuint& shaderID) {
    if (shaderID != 0) {
        glDeleteShader(shaderID);
        shaderID = 0;
    }
}

void compileShader(GLuint shaderID, const char* file) {
    // read shader code from the file
    std::string shaderCode;
    std::ifstream shaderStream(file, std::ios::in);
    if (shaderStream.is_open()) {
        std::string Line = "";
        while (getline(shaderStream, Line)) {
            shaderCode += "\n" + Line;
        }
        shaderStream.close();
    } else {
        throw runtime_error(string("Can't open shader file: ") + file);
    }

    GLint result = GL_FALSE;
    cout << "Compiling shader: " << file << endl;
    char const* sourcePointer = shaderCode.c_str();
    glShaderSource(shaderID, 1, &sourcePointer, NULL);
    glCompileShader(shaderID);

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    const std::string log = shaderInfoLog(shaderID);
    if (result != GL_TRUE) {
        throw runtime_error(string("Failed to compile shader: ") + file +
                            (log.empty() ? "" : string("\n") + log));
    }
    if (!log.empty()) {
        cout << log << endl;
    }
}

} // namespace

GLuint loadShaders(const char* vertexFilePath,
                   const char* fragmentFilePath,
                   const char* geometryFilePath) {
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint geometryShaderID = 0;
    if (geometryFilePath) {
        geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
    }

    try {
        compileShader(vertexShaderID, vertexFilePath);
        compileShader(fragmentShaderID, fragmentFilePath);
        if (geometryFilePath) compileShader(geometryShaderID, geometryFilePath);
    } catch (...) {
        deleteShader(vertexShaderID);
        deleteShader(fragmentShaderID);
        deleteShader(geometryShaderID);
        throw;
    }

    // Link the program
    cout << "Linking shaders... " << endl;
    GLuint programID = glCreateProgram();
    if (programID == 0) {
        deleteShader(vertexShaderID);
        deleteShader(fragmentShaderID);
        deleteShader(geometryShaderID);
        throw runtime_error("Failed to create an OpenGL shader program");
    }
    glAttachShader(programID, vertexShaderID);
    if (geometryFilePath)
        glAttachShader(programID, geometryShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    // Check the program
    GLint result = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    const std::string log = programInfoLog(programID);

    glDetachShader(programID, vertexShaderID);
    deleteShader(vertexShaderID);

    if (geometryFilePath) {
        glDetachShader(programID, geometryShaderID);
        deleteShader(geometryShaderID);
    }

    glDetachShader(programID, fragmentShaderID);
    deleteShader(fragmentShaderID);

    if (result != GL_TRUE) {
        glDeleteProgram(programID);
        throw runtime_error(
            string("Failed to link shader program (vertex: ") + vertexFilePath +
            ", fragment: " + fragmentFilePath + ")" +
            (log.empty() ? "" : string("\n") + log));
    }
    if (!log.empty()) {
        cout << log << endl;
    }

    cout << "Shader program complete." << endl;

    return programID;
}

#include "Skybox.h"

#include <SOIL.h>

#include <stdexcept>
#include <vector>

#include "common/shader.h"

static const float SKYBOX_VERTICES[] = {
	// positions
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

Skybox::Skybox(const std::vector<std::string>& faces)
	: m_drawable(nullptr),
	  m_cubemapTexture(0),
	  m_shaderProgram(0),
	  m_viewLocation(-1),
	  m_projectionLocation(-1),
	  m_flashLocation(-1),
	  m_timeLocation(-1),
	  m_moonLocation(-1),
	  m_calmLocation(-1)
{
	setupMesh();
	loadCubemap(faces);
	m_shaderProgram = loadShaders(ELEMENTAL_SHADER_DIR "/Skybox.vertexshader",
	                              ELEMENTAL_SHADER_DIR "/Skybox.fragmentshader");
	m_viewLocation = glGetUniformLocation(m_shaderProgram, "V");
	m_projectionLocation = glGetUniformLocation(m_shaderProgram, "P");
	m_flashLocation = glGetUniformLocation(m_shaderProgram, "uLightningFlash");
	m_timeLocation = glGetUniformLocation(m_shaderProgram, "uTime");
	m_moonLocation = glGetUniformLocation(m_shaderProgram, "uMoonDirection");
	m_calmLocation = glGetUniformLocation(m_shaderProgram, "uCalmProgress");

	glUseProgram(m_shaderProgram);
	GLint samplerLocation = glGetUniformLocation(m_shaderProgram, "skybox");
	if (samplerLocation >= 0)
	{
		glUniform1i(samplerLocation, 0);
	}
	glUseProgram(0);
}

Skybox::~Skybox()
{
	if (m_drawable)
	{
		delete m_drawable;
		m_drawable = nullptr;
	}

	if (m_cubemapTexture != 0)
	{
		glDeleteTextures(1, &m_cubemapTexture);
		m_cubemapTexture = 0;
	}

	if (m_shaderProgram != 0)
	{
		glDeleteProgram(m_shaderProgram);
		m_shaderProgram = 0;
	}
}

void Skybox::Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                  float lightningFlash,
                  float timeSeconds,
                  const glm::vec3& moonDirection,
                  float calmProgress)
{
	if (!m_drawable || m_cubemapTexture == 0 || m_shaderProgram == 0)
	{
		return;
	}

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(m_shaderProgram);

	glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(viewMatrix));
	if (m_viewLocation >= 0)
	{
		glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, &viewNoTranslation[0][0]);
	}
	if (m_projectionLocation >= 0)
	{
		glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
	}
	if (m_flashLocation >= 0)
	{
		glUniform1f(m_flashLocation, lightningFlash);
	}
	if (m_timeLocation >= 0)
	{
		glUniform1f(m_timeLocation, timeSeconds);
	}
	if (m_moonLocation >= 0)
	{
		glm::vec3 normalizedMoon = glm::normalize(moonDirection);
		glUniform3fv(m_moonLocation, 1, &normalizedMoon[0]);
	}
	if (m_calmLocation >= 0)
	{
		glUniform1f(m_calmLocation, calmProgress);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

	m_drawable->bind();
	m_drawable->draw();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glUseProgram(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

void Skybox::loadCubemap(const std::vector<std::string>& faces)
{
	if (faces.size() != 6)
	{
		throw std::runtime_error("Skybox cubemap requires exactly six images.");
	}

	glGenTextures(1, &m_cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

	for (unsigned int i = 0; i < faces.size(); ++i)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* data = SOIL_load_image(faces[i].c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
		if (!data)
		{
			glDeleteTextures(1, &m_cubemapTexture);
			m_cubemapTexture = 0;
			throw std::runtime_error("Failed to load cubemap face: " + faces[i]);
		}

		GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		SOIL_free_image_data(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::setupMesh()
{
	std::vector<glm::vec3> vertices;
	vertices.reserve(36);
	const size_t vertexCount = sizeof(SKYBOX_VERTICES) / sizeof(float);
	for (size_t i = 0; i < vertexCount; i += 3)
	{
		vertices.emplace_back(
			SKYBOX_VERTICES[i + 0],
			SKYBOX_VERTICES[i + 1],
			SKYBOX_VERTICES[i + 2]);
	}

	std::vector<glm::vec2> uvs(vertices.size(), glm::vec2(0.0f));
	std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));

	if (m_drawable)
	{
		delete m_drawable;
	}
	m_drawable = new Drawable(vertices, uvs, normals);
}

GLuint Skybox::loadShaders(const char* vertexPath, const char* fragmentPath)
{
	return ::loadShaders(vertexPath, fragmentPath);
}

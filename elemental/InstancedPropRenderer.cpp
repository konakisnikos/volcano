#include "InstancedPropRenderer.h"

#include <algorithm>
#include <cstddef>

#include <common/model.h>

InstancedPropRenderer::InstancedPropRenderer(Drawable* model)
    : m_model(model) {
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_model->verticesVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    if (!m_model->indexedNormals.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_model->normalsVBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_model->elementVBO);

    glGenBuffers(1, &m_matrixBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_matrixBuffer);
    const GLsizei matrixStride = static_cast<GLsizei>(sizeof(glm::mat4));
    for (GLuint column = 0; column < 4; ++column) {
        const GLuint attribute = 5 + column;
        glEnableVertexAttribArray(attribute);
        glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE, matrixStride,
                              reinterpret_cast<void*>(column * sizeof(glm::vec4)));
        glVertexAttribDivisor(attribute, 1);
    }

    glGenBuffers(1, &m_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer);
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glVertexAttribDivisor(9, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

InstancedPropRenderer::~InstancedPropRenderer() {
    glDeleteBuffers(1, &m_matrixBuffer);
    glDeleteBuffers(1, &m_colorBuffer);
    glDeleteVertexArrays(1, &m_vao);
}

void InstancedPropRenderer::draw(
    const std::vector<glm::mat4>& modelMatrices,
    const std::vector<glm::vec3>& colors) {
    const std::size_t instanceCount = std::min(modelMatrices.size(), colors.size());
    if (instanceCount == 0) return;

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_matrixBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instanceCount * sizeof(glm::mat4)),
                 modelMatrices.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instanceCount * sizeof(glm::vec3)),
                 colors.data(), GL_STREAM_DRAW);

    glDrawElementsInstanced(GL_TRIANGLES,
                            static_cast<GLsizei>(m_model->indices.size()),
                            GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(instanceCount));
}

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Types.hpp"

class Renderer
{
public:
    Renderer(const MeshData& mesh);
    ~Renderer();

    void draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int shaderProgram;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    int mvpLocation;
    int indexCount;

    unsigned int compileShader(unsigned int type, const char* source);
};
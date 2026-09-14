#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int shaderProgram;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    int mvpLocation;

    unsigned int compileShader(unsigned int type, const char* source);
};
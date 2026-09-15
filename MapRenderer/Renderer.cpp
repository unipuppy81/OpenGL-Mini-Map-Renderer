#include "Renderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <iostream>

using namespace std;

const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    vNormal = aNormal;
    vColor = aColor;

    gl_Position = uMVP * vec4(aPosition, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec3 vNormal;
in vec3 vColor;

out vec4 FragColor;

void main()
{
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -0.5));

    float diffuse = max(dot(normalize(vNormal), -lightDirection), 0.0);
    float lighting = 0.35 + diffuse * 0.65;

    FragColor = vec4(vColor * lighting, 1.0);
}
)";

Renderer::Renderer(const MeshData& mesh)
{
    indexCount = static_cast<int>(mesh.indices.size());

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}

unsigned int Renderer::compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        cout << "Shader compile error: " << log << '\n';
    }

    return shader;
}

void Renderer::draw(const glm::mat4& view, const glm::mat4& projection)
{
    glm::mat4 model(1.0f);
    glm::mat4 mvp = projection * view * model;

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}
#include "Renderer.hpp"
#include "Frustum.hpp"

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

Renderer::Renderer(const vector<MapTile>& tiles)
{
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");

    meshes.resize(tiles.size());

    for (size_t i = 0; i < tiles.size(); ++i)
        uploadMesh(meshes[i], tiles[i].mesh);
}

Renderer::~Renderer()
{
    for (GpuMesh& mesh : meshes)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }

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

FrameStats Renderer::draw(const vector<MapTile>& tiles, const glm::mat4& view, const glm::mat4& projection)
{
    FrameStats stats;

    glm::mat4 viewProjection = projection * view;
    Frustum frustum(viewProjection);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        if (!frustum.intersects(tiles[i].bounds)) continue;

        stats.visibleTiles++;

        const GpuMesh& mesh = meshes[i];
        if (mesh.indexCount == 0) continue;

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

        stats.drawCalls++;
        stats.renderedVertices += mesh.vertexCount;
    }

    return stats;
}
void Renderer::uploadMesh(GpuMesh& gpuMesh, const MeshData& mesh)
{
    gpuMesh.indexCount = static_cast<int>(mesh.indices.size());

    glGenVertexArrays(1, &gpuMesh.VAO);
    glGenBuffers(1, &gpuMesh.VBO);
    glGenBuffers(1, &gpuMesh.EBO);

    glBindVertexArray(gpuMesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
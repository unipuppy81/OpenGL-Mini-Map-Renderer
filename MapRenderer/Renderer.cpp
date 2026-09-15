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

        glDeleteVertexArrays(1, &routeMesh.VAO);
        glDeleteBuffers(1, &routeMesh.VBO);
        glDeleteBuffers(1, &routeMesh.EBO);

        glDeleteVertexArrays(1, &destinationMesh.VAO);
        glDeleteBuffers(1, &destinationMesh.VBO);
        glDeleteBuffers(1, &destinationMesh.EBO);

        glDeleteVertexArrays(1, &vehicleMesh.VAO);
        glDeleteBuffers(1, &vehicleMesh.VBO);
        glDeleteBuffers(1, &vehicleMesh.EBO);
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

FrameStats Renderer::draw(const vector<MapTile>& tiles, const glm::mat4& view, const glm::mat4& projection, bool cullingEnabled, glm::vec2 vehiclePosition, float vehicleHeading)
{
    FrameStats stats;

    glm::mat4 viewProjection = projection * view;
    Frustum frustum(viewProjection);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        if (cullingEnabled && !frustum.intersects(tiles[i].bounds)) continue;

        stats.visibleTiles++;
        stats.renderedBuildings += tiles[i].buildingCount;
        stats.renderedRoadSegments += tiles[i].roadSegmentCount;

        const GpuMesh& mesh = meshes[i];
        if (mesh.indexCount == 0) continue;

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

        stats.drawCalls++;
        stats.renderedVertices += mesh.vertexCount;
    }

    // route
    if (routeMesh.indexCount > 0) 
    {
        glBindVertexArray(routeMesh.VAO);
        glDrawElements(GL_TRIANGLES, routeMesh.indexCount, GL_UNSIGNED_INT, nullptr);

        stats.drawCalls++;
        stats.renderedVertices += routeMesh.vertexCount;
    }

    // destination
    if (destinationMesh.indexCount > 0) {
        glBindVertexArray(destinationMesh.VAO);
        glDrawElements(GL_TRIANGLES, destinationMesh.indexCount, GL_UNSIGNED_INT, nullptr);

        stats.drawCalls++;
        stats.renderedVertices += destinationMesh.vertexCount;
    }

    // vehicle
    updateVehicle(vehiclePosition, vehicleHeading);
    if (vehicleMesh.indexCount > 0) {
        glBindVertexArray(vehicleMesh.VAO);
        glDrawElements(GL_TRIANGLES, vehicleMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        stats.drawCalls++;
        stats.renderedVertices += vehicleMesh.vertexCount;
    }

    return stats;
}

void Renderer::uploadMesh(GpuMesh& gpuMesh, const MeshData& mesh)
{
    if (gpuMesh.VAO != 0) {
        glDeleteVertexArrays(1, &gpuMesh.VAO);
        glDeleteBuffers(1, &gpuMesh.VBO);
        glDeleteBuffers(1, &gpuMesh.EBO);
        gpuMesh = {};
    }

    gpuMesh.indexCount = static_cast<int>(mesh.indices.size());
    gpuMesh.vertexCount = mesh.vertices.size();

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

void Renderer::uploadRoute(const vector<glm::vec2>& points, float width)
{
    MeshData route;
    glm::vec3 color(1.0f, 0.48f, 0.08f);
    float y = 0.16f;

    for (size_t i = 1; i < points.size(); ++i) {
        glm::vec2 delta = points[i] - points[i - 1];
        if (glm::dot(delta, delta) < 0.0001f) continue;

        glm::vec2 offset = glm::normalize(glm::vec2(-delta.y, delta.x)) * width * 0.5f;
        unsigned int base = static_cast<unsigned int>(route.vertices.size());

        route.vertices.push_back({ {points[i - 1].x + offset.x, y, points[i - 1].y + offset.y}, {0, 1, 0}, color });
        route.vertices.push_back({ {points[i - 1].x - offset.x, y, points[i - 1].y - offset.y}, {0, 1, 0}, color });
        route.vertices.push_back({ {points[i].x - offset.x, y, points[i].y - offset.y}, {0, 1, 0}, color });
        route.vertices.push_back({ {points[i].x + offset.x, y, points[i].y + offset.y}, {0, 1, 0}, color });

        route.indices.insert(route.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }

    uploadMesh(routeMesh, route);
}

void Renderer::uploadDestination(glm::vec2 position, float buildingHeight)
{
    float baseY = buildingHeight + 4.0f;
    glm::vec3 color(1.0f, 0.12f, 0.24f);

    MeshData marker;

    marker.vertices = {
        {{position.x, baseY + 8.0f, position.y}, {0, 1, 0}, color},
        {{position.x - 4.0f, baseY, position.y - 4.0f}, {0, 1, 0}, color},
        {{position.x + 4.0f, baseY, position.y - 4.0f}, {0, 1, 0}, color},
        {{position.x + 4.0f, baseY, position.y + 4.0f}, {0, 1, 0}, color},
        {{position.x - 4.0f, baseY, position.y + 4.0f}, {0, 1, 0}, color}
    };

    marker.indices = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1 };

    uploadMesh(destinationMesh, marker);
}

void Renderer::updateVehicle(glm::vec2 position, float heading)
{
    float c = cos(heading);
    float s = sin(heading);

    auto transform = [&](glm::vec2 local) {
        return glm::vec3(
            position.x + local.x * c - local.y * s,
            0.42f,
            position.y + local.x * s + local.y * c
        );
        };

    glm::vec3 color(0.16f, 0.88f, 1.0f);

    MeshData vehicle;
    vehicle.vertices = {
        {transform({5.2f, 0.0f}), {0, 1, 0}, color},
        {transform({-4.0f, -3.4f}), {0, 1, 0}, color},
        {transform({-4.0f, 3.4f}), {0, 1, 0}, color}
    };
    vehicle.indices = { 0, 1, 2 };

    if (vehicleMesh.VAO == 0) 
        uploadMesh(vehicleMesh, vehicle);
    else 
    {
        glBindBuffer(GL_ARRAY_BUFFER, vehicleMesh.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vehicle.vertices.size() * sizeof(Vertex), vehicle.vertices.data());
    }
}
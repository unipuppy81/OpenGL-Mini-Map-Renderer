#pragma once

#include <glm/glm.hpp>
#include <vector>

struct BuildingData
{
    std::vector<glm::vec2> polygon;
    float height = 10.0f;
};

struct RoadData
{
    std::vector<glm::vec2> points;
    float width = 2.0f;
};

struct MapData
{
    std::vector<BuildingData> buildings;
    std::vector<RoadData> roads;
};

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 color{};
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};
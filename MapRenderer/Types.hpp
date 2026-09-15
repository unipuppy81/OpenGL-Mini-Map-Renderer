#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <limits>


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

    void append(const MeshData& other)
    {
        unsigned int base = static_cast<unsigned int>(vertices.size());

        vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());

        for (unsigned int index : other.indices)
            indices.push_back(base + index);
    }
};

struct AABB
{
    glm::vec3 min{ std::numeric_limits<float>::max() };
    glm::vec3 max{ std::numeric_limits<float>::lowest() };
};

struct MapTile
{
    glm::ivec2 coordinate{};
    AABB bounds{};
    MeshData mesh{};
    size_t buildingCount = 0;
    size_t roadSegmentCount = 0;
};

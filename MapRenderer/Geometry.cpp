#include "Geometry.hpp"
#include "Types.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cmath>

using namespace std;

MeshData Geometry::createBuilding(const BuildingData& building)
{
    MeshData mesh;

    const vector<glm::vec2>& polygon = building.polygon;

    if (polygon.size() < 3) return mesh;

    glm::vec3 roofColor(0.7f, 0.75f, 0.8f);
    glm::vec3 wallColor(0.5f, 0.55f, 0.6f);

    // roof vertices
    for (const glm::vec2& point : polygon)
    {
        mesh.vertices.push_back({
            glm::vec3(point.x, building.height, point.y),
            glm::vec3(0.0f, 1.0f, 0.0f),
            roofColor
            });
    }

    // roof indices
    // 현재 단계에서는 convex polygon 기준 triangle fan
    for (unsigned int i = 1; i + 1 < polygon.size(); ++i)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i);
        mesh.indices.push_back(i + 1);
    }

    // walls
    for (size_t i = 0; i < polygon.size(); ++i)
    {
        const glm::vec2& a = polygon[i];
        const glm::vec2& b = polygon[(i + 1) % polygon.size()];

        glm::vec3 bottomA(a.x, 0.0f, a.y);
        glm::vec3 bottomB(b.x, 0.0f, b.y);
        glm::vec3 topA(a.x, building.height, a.y);
        glm::vec3 topB(b.x, building.height, b.y);

        glm::vec3 edge = bottomB - bottomA;

        if (glm::length(edge) < 0.0001f) continue;

        glm::vec3 normal = glm::normalize(glm::vec3(edge.z, 0.0f, -edge.x));

        unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

        mesh.vertices.push_back({ bottomA, normal, wallColor });
        mesh.vertices.push_back({ bottomB, normal, wallColor });
        mesh.vertices.push_back({ topB, normal, wallColor });
        mesh.vertices.push_back({ topA, normal, wallColor });

        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);

        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    return mesh;
}

MeshData Geometry::createRoad(const RoadData& road)
{
    MeshData mesh;

    for (size_t i = 0; i + 1 < road.points.size(); ++i)
        mesh.append(createRoadSegment(road.points[i], road.points[i + 1], road.width));

    return mesh;
}

MeshData Geometry::createRoadSegment(glm::vec2 a, glm::vec2 b, float width)
{
    MeshData mesh;

    glm::vec2 direction = b - a;
    if (glm::length(direction) < 0.0001f) return mesh;

    direction = glm::normalize(direction);

    glm::vec2 perpendicular(-direction.y, direction.x);
    glm::vec2 offset = perpendicular * width * 0.5f;

    glm::vec2 leftA = a + offset;
    glm::vec2 rightA = a - offset;
    glm::vec2 leftB = b + offset;
    glm::vec2 rightB = b - offset;

    glm::vec3 color(0.18f, 0.18f, 0.20f);

    mesh.vertices = {
        {{leftA.x, 0.05f, leftA.y}, {0, 1, 0}, color},
        {{rightA.x, 0.05f, rightA.y}, {0, 1, 0}, color},
        {{rightB.x, 0.05f, rightB.y}, {0, 1, 0}, color},
        {{leftB.x, 0.05f, leftB.y}, {0, 1, 0}, color}
    };

    mesh.indices = { 0, 1, 2, 0, 2, 3 };

    return mesh;
}

TileManager::TileManager(float tileSize) : tileSize(tileSize)
{
}

const vector<MapTile>& TileManager::getTiles() const
{
    return tiles;
}

glm::ivec2 TileManager::getCoordinate(glm::vec2 point) const
{
    return {
        static_cast<int>(floor(point.x / tileSize)),
        static_cast<int>(floor(point.y / tileSize))
    };
}

MapTile& TileManager::getOrCreate(glm::ivec2 coordinate)
{
    unsigned long long key =
        (static_cast<unsigned long long>(static_cast<unsigned int>(coordinate.x)) << 32) |
        static_cast<unsigned int>(coordinate.y);

    auto found = tileLookup.find(key);
    if (found != tileLookup.end()) return tiles[found->second];

    MapTile tile;
    tile.coordinate = coordinate;

    tile.bounds.min = {
        coordinate.x * tileSize,
        0.0f,
        coordinate.y * tileSize
    };

    tile.bounds.max = {
        (coordinate.x + 1) * tileSize,
        1.0f,
        (coordinate.y + 1) * tileSize
    };

    size_t index = tiles.size();

    tiles.push_back(tile);
    tileLookup[key] = index;

    return tiles.back();
}


void TileManager::build(const MapData& mapData)
{
    tiles.clear();
    tileLookup.clear();

    for (const BuildingData& building : mapData.buildings)
    {
        if (building.polygon.empty()) continue;

        glm::vec2 center(0.0f);

        for (glm::vec2 point : building.polygon)
            center += point;

        center /= static_cast<float>(building.polygon.size());

        MapTile& tile = getOrCreate(getCoordinate(center));

        tile.mesh.append(Geometry::createBuilding(building));
        tile.buildingCount++;

        if (building.height > tile.bounds.max.y)
            tile.bounds.max.y = building.height;
    }

    for (const RoadData& road : mapData.roads)
    {
        for (size_t i = 0; i + 1 < road.points.size(); ++i)
        {
            glm::vec2 a = road.points[i];
            glm::vec2 b = road.points[i + 1];
            glm::vec2 center = (a + b) * 0.5f;

            MapTile& tile = getOrCreate(getCoordinate(center));

            tile.mesh.append(Geometry::createRoadSegment(a, b, road.width));
            tile.roadSegmentCount++;
        }
    }
}
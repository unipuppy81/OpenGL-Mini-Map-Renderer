#pragma once

#include "Types.hpp"
#include <unordered_map>

class Geometry
{
public:
    static MeshData createBuilding(const BuildingData& building);
    static MeshData createRoad(const RoadData& road);
    static MeshData createRoadSegment(glm::vec2 a, glm::vec2 b, float width);
};

class TileManager
{
public:
    TileManager(float tileSize = 80.0f);

    void build(const MapData& mapData);
    const std::vector<MapTile>& getTiles() const;

private:
    glm::ivec2 getCoordinate(glm::vec2 point) const;
    MapTile& getOrCreate(glm::ivec2 coordinate);

    float tileSize;
    std::vector<MapTile> tiles;
    std::unordered_map<unsigned long long, size_t> tileLookup;
};
#include "GeoJsonLoader.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

#include <iostream>
#include <filesystem>

using json = nlohmann::json;

constexpr double METERS_PER_LATITUDE = 110540.0;
constexpr double METERS_PER_LONGITUDE = 111320.0;
constexpr double PI = 3.14159265358979323846;

void convertToLocalMeters(MapData& mapData)
{
    double minLon = std::numeric_limits<double>::max();
    double maxLon = std::numeric_limits<double>::lowest();
    double minLat = std::numeric_limits<double>::max();
    double maxLat = std::numeric_limits<double>::lowest();

    for (const auto& building : mapData.buildings)
    {
        for (const auto& point : building.polygon)
        {
            minLon = std::min(minLon, (double)point.x);
            maxLon = std::max(maxLon, (double)point.x);
            minLat = std::min(minLat, (double)point.y);
            maxLat = std::max(maxLat, (double)point.y);
        }
    }

    for (const auto& road : mapData.roads)
    {
        for (const auto& point : road.points)
        {
            minLon = std::min(minLon, (double)point.x);
            maxLon = std::max(maxLon, (double)point.x);
            minLat = std::min(minLat, (double)point.y);
            maxLat = std::max(maxLat, (double)point.y);
        }
    }

    if (minLon > maxLon) return;

    double originLon = (minLon + maxLon) * 0.5;
    double originLat = (minLat + maxLat) * 0.5;

    double longitudeScale = METERS_PER_LONGITUDE * std::cos(originLat * PI / 180.0);

    for (auto& building : mapData.buildings)
    {
        for (auto& point : building.polygon)
        {
            point.x = (float)((point.x - originLon) * longitudeScale);
            point.y = (float)((point.y - originLat) * METERS_PER_LATITUDE);
        }
    }

    for (auto& road : mapData.roads)
    {
        for (auto& point : road.points)
        {
            point.x = (float)((point.x - originLon) * longitudeScale);
            point.y = (float)((point.y - originLat) * METERS_PER_LATITUDE);
        }
    }
}

float getFloatProperty(const json& properties, const char* name, float defaultValue)
{
    if (!properties.contains(name)) return defaultValue;

    const auto& value = properties[name];

    if (value.is_number()) return value.get<float>();

    return defaultValue;
}

void loadPolygon(const json& coordinates, const json& properties, MapData& mapData)
{
    if (!coordinates.is_array() || coordinates.empty()) return;

    const auto& ring = coordinates[0];

    if (!ring.is_array()) return;

    BuildingData building;

    building.height = getFloatProperty(properties, "height", 10.0f);

    for (const auto& point : ring)
    {
        if (!point.is_array() || point.size() < 2) continue;

        float longitude = point[0].get<float>();
        float latitude = point[1].get<float>();

        building.polygon.emplace_back(longitude, latitude);
    }

    if (building.polygon.size() >= 2 &&
        building.polygon.front() == building.polygon.back())
    {
        building.polygon.pop_back();
    }

    if (building.polygon.size() >= 3) mapData.buildings.push_back(building);
}

void loadLineString(const json& coordinates, const json& properties, MapData& mapData)
{
    if (!coordinates.is_array()) return;

    RoadData road;

    road.width = getFloatProperty(properties, "width", 2.0f);

    for (const auto& point : coordinates)
    {
        if (!point.is_array() || point.size() < 2) continue;

        float longitude = point[0].get<float>();
        float latitude = point[1].get<float>();

        road.points.emplace_back(longitude, latitude);
    }

    if (road.points.size() >= 2) mapData.roads.push_back(road);
}

MapData GeoJsonLoader::load(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open()) throw std::runtime_error("Failed to open GeoJSON: " + path);

    json root;
    file >> root;

    if (root.value("type", "") != "FeatureCollection")
        throw std::runtime_error("GeoJSON root is not FeatureCollection");

    MapData mapData;

    if (!root.contains("features")) return mapData;

    for (const auto& feature : root["features"])
    {
        if (!feature.contains("geometry")) continue;
        if (feature["geometry"].is_null()) continue;

        const auto& geometry = feature["geometry"];

        std::string type = geometry.value("type", "");

        if (!geometry.contains("coordinates")) continue;

        const auto& coordinates = geometry["coordinates"];

        json properties = json::object();

        if (feature.contains("properties") && feature["properties"].is_object())
            properties = feature["properties"];

        if (type == "Polygon")
        {
            loadPolygon(coordinates, properties, mapData);
        }
        else if (type == "LineString")
        {
            loadLineString(coordinates, properties, mapData);
        }
    }

    convertToLocalMeters(mapData);

    return mapData;
}
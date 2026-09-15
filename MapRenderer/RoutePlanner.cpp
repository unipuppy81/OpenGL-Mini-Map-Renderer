#include "RoutePlanner.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <utility>

using namespace std;

namespace {
    struct GraphNode {
        glm::vec2 position{};
        vector<pair<size_t, float>> edges;
    };

    uint64_t snapKey(glm::vec2 point, float snapDistance)
    {
        int32_t x = static_cast<int32_t>(lround(point.x / snapDistance));
        int32_t y = static_cast<int32_t>(lround(point.y / snapDistance));
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
    }

    glm::vec2 centroid(const BuildingData& building)
    {
        glm::vec2 result(0.0f);
        for (glm::vec2 point : building.polygon) result += point;
        return result / static_cast<float>(building.polygon.size());
    }

    float cross2d(glm::vec2 a, glm::vec2 b, glm::vec2 c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    bool segmentsIntersect(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d)
    {
        float abC = cross2d(a, b, c);
        float abD = cross2d(a, b, d);
        float cdA = cross2d(c, d, a);
        float cdB = cross2d(c, d, b);
        return abC * abD < -0.0001f && cdA * cdB < -0.0001f;
    }

    bool pointInPolygon(glm::vec2 point, const vector<glm::vec2>& polygon)
    {
        if (polygon.size() < 3) return false;

        bool inside = false;

        for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
            glm::vec2 a = polygon[i];
            glm::vec2 b = polygon[j];

            bool crosses = (a.y > point.y) != (b.y > point.y);
            if (crosses && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
        }

        return inside;
    }

    bool blockedByBuilding(glm::vec2 a, glm::vec2 b, const vector<BuildingData>& buildings)
    {
        glm::vec2 midpoint = (a + b) * 0.5f;

        for (const BuildingData& building : buildings) {
            if (pointInPolygon(midpoint, building.polygon)) return true;

            for (size_t i = 0; i < building.polygon.size(); ++i) {
                glm::vec2 c = building.polygon[i];
                glm::vec2 d = building.polygon[(i + 1) % building.polygon.size()];
                if (segmentsIntersect(a, b, c, d)) return true;
            }
        }

        return false;
    }

    size_t nearestNode(const vector<GraphNode>& nodes, glm::vec2 point)
    {
        size_t best = 0;
        float bestDistance = numeric_limits<float>::max();

        for (size_t i = 0; i < nodes.size(); ++i) {
            glm::vec2 delta = nodes[i].position - point;
            float distance = glm::dot(delta, delta);

            if (distance < bestDistance) {
                bestDistance = distance;
                best = i;
            }
        }

        return best;
    }
}

PlannedRoute RoutePlanner::plan(const MapData& mapData, glm::vec2 startPosition, size_t destinationBuildingIndex) const
{
    if (mapData.roads.empty()) throw runtime_error("Route planning requires roads");
    if (destinationBuildingIndex >= mapData.buildings.size()) throw runtime_error("Destination building index out of range");

    vector<GraphNode> nodes;
    unordered_map<uint64_t, size_t> nodeLookup;

    auto nodeFor = [&](glm::vec2 point) {
        uint64_t key = snapKey(point, snapDistance);

        auto found = nodeLookup.find(key);
        if (found != nodeLookup.end()) return found->second;

        size_t index = nodes.size();
        nodes.push_back({ point, {} });
        nodeLookup[key] = index;
        return index;
        };

    for (const RoadData& road : mapData.roads) {
        for (size_t i = 1; i < road.points.size(); ++i) {
            glm::vec2 a = road.points[i - 1];
            glm::vec2 b = road.points[i];

            if (blockedByBuilding(a, b, mapData.buildings)) continue;

            size_t from = nodeFor(a);
            size_t to = nodeFor(b);
            if (from == to) continue;

            float cost = glm::length(nodes[to].position - nodes[from].position);
            nodes[from].edges.push_back({ to, cost });
            nodes[to].edges.push_back({ from, cost });
        }
    }

    if (nodes.empty()) throw runtime_error("Road graph has no usable segments");

    glm::vec2 destination = centroid(mapData.buildings[destinationBuildingIndex]);
    size_t start = nearestNode(nodes, startPosition);
    size_t goal = nearestNode(nodes, destination);

    float infinity = numeric_limits<float>::max();
    vector<float> distance(nodes.size(), infinity);
    vector<size_t> previous(nodes.size(), nodes.size());

    using QueueItem = pair<float, size_t>;
    priority_queue<QueueItem, vector<QueueItem>, greater<>> queue;

    distance[start] = 0.0f;
    queue.push({ 0.0f, start });

    while (!queue.empty()) {
        auto [currentDistance, current] = queue.top();
        queue.pop();

        if (currentDistance > distance[current]) continue;
        if (current == goal) break;

        for (auto [next, cost] : nodes[current].edges) {
            float candidate = currentDistance + cost;
            if (candidate >= distance[next]) continue;

            distance[next] = candidate;
            previous[next] = current;
            queue.push({ candidate, next });
        }
    }

    if (distance[goal] == infinity) throw runtime_error("No route to destination");

    vector<glm::vec2> path;

    for (size_t node = goal;; node = previous[node]) {
        path.push_back(nodes[node].position);
        if (node == start) break;
    }

    reverse(path.begin(), path.end());

    return { move(path), destination, mapData.buildings[destinationBuildingIndex].height };
}
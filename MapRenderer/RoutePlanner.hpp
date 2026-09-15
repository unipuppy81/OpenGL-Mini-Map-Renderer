#pragma once

#include "Types.hpp"
#include <cstddef>
#include <vector>

struct PlannedRoute {
    std::vector<glm::vec2> roadPath;
    glm::vec2 destinationBuilding{};
    float destinationHeight = 0.0f;
};

class RoutePlanner {
public:
    PlannedRoute plan(const MapData& mapData, glm::vec2 startPosition, std::size_t destinationBuildingIndex) const;

private:
    float snapDistance = 1.5f;
};
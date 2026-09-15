#include "Geometry.hpp"
#include "Types.hpp"

#include <glm/glm.hpp>
#include <vector>

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
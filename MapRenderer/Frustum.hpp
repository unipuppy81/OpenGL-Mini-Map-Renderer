#pragma once

#include <array>
#include <glm/glm.hpp>

#include "Types.hpp"

class Frustum
{
public:
    Frustum(const glm::mat4& viewProjection)
    {
        glm::mat4 m = glm::transpose(viewProjection);

        planes[0] = m[3] + m[0];
        planes[1] = m[3] - m[0];
        planes[2] = m[3] + m[1];
        planes[3] = m[3] - m[1];
        planes[4] = m[3] + m[2];
        planes[5] = m[3] - m[2];

        for (glm::vec4& plane : planes)
        {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) plane /= length;
        }
    }

    bool intersects(const AABB& box) const
    {
        for (const glm::vec4& plane : planes)
        {
            glm::vec3 normal(plane);

            glm::vec3 positive(
                normal.x >= 0.0f ? box.max.x : box.min.x,
                normal.y >= 0.0f ? box.max.y : box.min.y,
                normal.z >= 0.0f ? box.max.z : box.min.z
            );

            if (glm::dot(normal, positive) + plane.w < 0.0f)
                return false;
        }

        return true;
    }

private:
    std::array<glm::vec4, 6> planes{};
};
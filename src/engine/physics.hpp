#pragma once

#include <glm/glm.hpp>
#include <algorithm>

struct AABB {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);

    bool Intersects(const AABB& other) const {
        return  (min.x <= other.max.x && max.x >= other.min.x) &&
                (min.y <= other.max.y && max.y >= other.min.y) &&
                (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool Contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
               (point.y >= min.y && point.y <= max.y) &&
               (point.z >= min.z && point.z <= max.z);
    }

    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
    glm::vec3 GetSize() const { return max - min; }
};

struct Cylinder {
    glm::vec3 position;
    float radius;
    float height;
};

inline bool CheckCylinderAABB(const Cylinder& cyl, const AABB& box, glm::vec3& resolution) {
    float closestX = std::max(box.min.x, std::min(cyl.position.x, box.max.x));
    float closestY = std::max(box.min.y, std::min(cyl.position.y + (cyl.height/2), box.max.y)); // Approx Y check
    float closestZ = std::max(box.min.z, std::min(cyl.position.z, box.max.z));

    float distanceX = cyl.position.x - closestX;
    float distanceZ = cyl.position.z - closestZ;

    bool overlapY = (cyl.position.y < box.max.y && cyl.position.y + cyl.height > box.min.y);
    if (!overlapY) return false;

    float distanceSquared = (distanceX * distanceX) + (distanceZ * distanceZ);
    if (distanceSquared < (cyl.radius * cyl.radius)) {
        // COLLISION DETECTED

        // Calculate smooth push-back direction
        float dist = sqrt(distanceSquared);
        if (dist == 0.0f) {
            // Center is exactly inside box (rare), push out +X arbitrarily
            resolution = glm::vec3(1, 0, 0) * (cyl.radius - dist);
        } else {
            glm::vec3 normal = glm::vec3(distanceX, 0, distanceZ) / dist;
            resolution = normal * (cyl.radius - dist);
        }
        return true;
    }
    return false;
}
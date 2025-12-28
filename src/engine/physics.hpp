#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

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
    // 1. Early AABB Rejection (Optimization)
    if (cyl.position.x - cyl.radius > box.max.x || cyl.position.x + cyl.radius < box.min.x ||
        cyl.position.z - cyl.radius > box.max.z || cyl.position.z + cyl.radius < box.min.z ||
        cyl.position.y > box.max.y || cyl.position.y + cyl.height < box.min.y) {
        return false;
    }

    // 2. Find closest point on AABB to Cylinder Center (XZ plane only)
    float closestX = std::max(box.min.x, std::min(cyl.position.x, box.max.x));
    float closestZ = std::max(box.min.z, std::min(cyl.position.z, box.max.z));

    // 3. Distance from cylinder center to that point
    float distX = cyl.position.x - closestX;
    float distZ = cyl.position.z - closestZ;
    float distSq = distX * distX + distZ * distZ;

    // 4. Check if we are colliding in XZ
    // Note: If distSq is 0, the center is inside the box. That counts as collision.
    bool centerInsideXZ = (distSq < 0.00001f);
    if (!centerInsideXZ && distSq > (cyl.radius * cyl.radius)) {
        return false;
    }

    // --- COLLISION CONFIRMED - RESOLVE IT ---

    // 5. Calculate Y Penetration (Floor/Ceiling)
    // We assume 'position' is the bottom of the cylinder
    float distToTop = box.max.y - cyl.position.y;                   // Push Up (Floor)
    float distToBottom = box.min.y - (cyl.position.y + cyl.height); // Push Down (Ceiling)

    // Choose the smaller Y push
    float penetrationY = (std::abs(distToTop) < std::abs(distToBottom)) ? distToTop : distToBottom;

    // 6. Calculate XZ Penetration (Wall)
    float penetrationXZ = 0.0f;
    glm::vec3 normalXZ(0.0f);

    if (centerInsideXZ) {
        // We are deep inside the AABB. Find the nearest edge.
        float dMinX = cyl.position.x - box.min.x;
        float dMaxX = box.max.x - cyl.position.x;
        float dMinZ = cyl.position.z - box.min.z;
        float dMaxZ = box.max.z - cyl.position.z;

        float minDim = std::min({dMinX, dMaxX, dMinZ, dMaxZ});

        if (minDim == dMinX) normalXZ = glm::vec3(-1, 0, 0);
        else if (minDim == dMaxX) normalXZ = glm::vec3(1, 0, 0);
        else if (minDim == dMinZ) normalXZ = glm::vec3(0, 0, -1);
        else normalXZ = glm::vec3(0, 0, 1);

        penetrationXZ = minDim + cyl.radius;
    } else {
        // We are hitting the corner/edge
        float dist = std::sqrt(distSq);
        normalXZ = glm::vec3(distX, 0, distZ) / dist;
        penetrationXZ = cyl.radius - dist;
    }

    // 7. Choose the "Path of Least Resistance"
    // If Y penetration is smaller than XZ penetration, it's a floor/ceiling hit.
    if (std::abs(penetrationY) < std::abs(penetrationXZ)) {
        resolution = glm::vec3(0, penetrationY, 0);
    } else {
        resolution = normalXZ * penetrationXZ;
    }

    return true;
}

class PhysicsDebugDrawer {
public:
    static void Init();
    static void DrawAABB(const AABB& box, glm::vec3 color);
    static void DrawCylinder(const Cylinder& cyl, glm::vec3 color);

    // Call this at the very end of your Level Render function
    static void Render(const glm::mat4& view, const glm::mat4& projection);

private:
    // We store lines in batches based on color to minimize draw calls
    struct LineBatch {
        glm::vec3 color;
        std::vector<float> vertices; // x,y,z, x,y,z...
    };

    static std::vector<LineBatch> m_Batches;
    static unsigned int m_VAO, m_VBO;
};
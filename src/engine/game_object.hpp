#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "physics.hpp"
#include "resource_manager.hpp"

struct Transform {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
    glm::vec3 Scale    = glm::vec3(1.0f);

    glm::mat4 GetMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, Scale);
        return model;
    }
};

enum class ObjectType {
    Base,
    Mesh,
    Light,
    Plane,
    COUNT
};

struct GameObject {
    Transform transform;
    ObjectType type = ObjectType::Base;
    bool visible = true;

    AABB collider;
    AABB localBounds;
    float collisionRadius = 0.0f;
    float collisionHeight = 0.0f;
    bool hasCollision = false;

    GameObject(ObjectType t = ObjectType::Base) : type(t) {
        localBounds.min = glm::vec3(-0.5f);
        localBounds.max = glm::vec3(0.5f);
    }
    virtual ~GameObject() = default;

    void UpdateSelfAndChild() {
        // Correctly transform AABB with scale and position, preserving local offset
        glm::vec3 min = localBounds.min * transform.Scale;
        glm::vec3 max = localBounds.max * transform.Scale;

        // Handle negative scale if necessary (swap min/max components)
        if (min.x > max.x) std::swap(min.x, max.x);
        if (min.y > max.y) std::swap(min.y, max.y);
        if (min.z > max.z) std::swap(min.z, max.z);

        collider.min = transform.Position + min;
        collider.max = transform.Position + max;
    }
};

struct MeshObject : public GameObject {
    Model* modelResource = nullptr;

    MeshObject() : GameObject(ObjectType::Mesh) {}
};

struct PlaneObject : public GameObject {
    unsigned int textureID = 0;

    PlaneObject() : GameObject(ObjectType::Plane) {}
};

struct PointLight : public GameObject {
    glm::vec3 Color = glm::vec3(1.0f);
    float Intensity = 1.0f;
    float Radius = 50.0f;

    PointLight() : GameObject(ObjectType::Light) {}
};
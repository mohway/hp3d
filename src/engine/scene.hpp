#pragma once
#include <vector>
#include <memory>

#include "../arena.hpp"
#include "../camera.hpp"
#include "renderer.hpp"
#include "game_object.hpp"
#include "GLFW/glfw3.h"

// Forward declaration
class Jolt_Impl;

class Scene {
public:
    virtual ~Scene() { m_SceneArena.destroy(); }

    virtual void Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void ProcessInput(GLFWwindow* window, float dt) = 0;

    virtual void Render(Renderer& renderer, int screenWidth, int screenHeight) {
        renderer.RenderScene(m_Camera, m_Objects, screenWidth, screenHeight);
    }

    // Allow passing physics system to scene
    void SetPhysics(Jolt_Impl* physics) { m_Physics = physics; }

    std::vector<GameObject*>& GetObjects() { return m_Objects; }

    Camera m_Camera { glm::vec3(0.0f, 1.0f, 3.0f) };

protected:
    template <typename T>
    T* CreateObject() {
        T* obj = m_SceneArena.alloc<T>();
        new(obj) T();
        m_Objects.push_back(obj);
        return obj;
    }

    Arena m_SceneArena;

    std::vector<GameObject*> m_Objects;
    Jolt_Impl* m_Physics = nullptr;
};

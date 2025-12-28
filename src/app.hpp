#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "engine/window.hpp"
#include "arena.hpp"
#include "camera.hpp"
#include "engine/resource_manager.hpp"
#include "engine/renderer.hpp"
#include "engine/scene.hpp"
#include "engine/3p/jolt_impl.hpp"

// class Renderer;
// class Camera;
// class Input;

class App {
public:
    App(const std::string& title, int width, int height);
    ~App();
    void run();

private:
    void init();
    void update(float dt);
    void render();
    void process_input(float dt);

    std::unique_ptr<Window> m_Window;
    Renderer m_Renderer;
    std::unique_ptr<Scene> m_CurrentScene;
    bool m_IsRunning;
    bool m_EditorActive = false;

    // Physics
    Jolt_Impl m_Physics;

    // ====== GLOBALS

    struct G {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float playerZ = 0.0f;
    } g;

    Arena m_FrameArena;

    ShaderProgram shader_program;
    unsigned int m_vao, m_vbo;
    int m_FloorVertexCount;

    // Mouse State
    float m_LastX = 400, m_LastY = 300;
    bool m_FirstMouse = true;

    // The loaded model
    Model m_HarryModel;

    unsigned int m_ShadowMapFBO;
    unsigned int m_ShadowMapTexture;
    unsigned int m_ShadowShader;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // Crisp shadows
};
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "arena.hpp"
#include "camera.hpp"
#include "engine/resource_manager.hpp"

// class Renderer;
// class Camera;
// class Input;

class App {
public:
    App(const std::string& title, int width, int height);
    ~App();

    void run();

    struct SubMesh {
        unsigned int vao;
        unsigned int vbo;
        unsigned int textureID;
        int vertexCount;
    };

    // A "Model" is just a list of parts
    using Model = std::vector<SubMesh>;

private:
    void init();
    void update(float dt);
    void render();
    void process_input(float dt);

    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    bool m_IsRunning;

    // std::unique_ptr<Renderer> m_Renderer;
    // std::unique_ptr<Camera> m_Camera;

    // ====== GLOBALS

    struct G {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float playerZ = 0.0f;
    } g;

    // ====== ARENAS
    Arena m_LevelArena;
    Arena m_FrameArena;

    ShaderProgram shader_program;
    unsigned int m_vao, m_vbo;
    int m_FloorVertexCount;

    // FBO Stuff
    unsigned int m_FBO;         // The Framebuffer Object
    unsigned int m_TexColorBuffer; // The Texture we render to
    unsigned int m_RBO;         // The Depth Buffer for the FBO

    // Screen Quad Stuff
    unsigned int m_ScreenVAO, m_ScreenVBO;
    unsigned int m_ScreenShader; // The compiled screen.vert/frag

    // Settings
    const int INTERNAL_WIDTH = 320;
    const int INTERNAL_HEIGHT = 240;

    // Camera System
    Camera m_Camera;

    // Mouse State
    float m_LastX = 400, m_LastY = 300;
    bool m_FirstMouse = true;

    unsigned int m_FloorTexture;

    Model load_model(const char* objPath);

    // The loaded model
    Model m_Model;

    unsigned int m_ShadowMapFBO;
    unsigned int m_ShadowMapTexture;
    unsigned int m_ShadowShader;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // Crisp shadows
};
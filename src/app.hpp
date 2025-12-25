#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "arena.hpp"
#include "camera.hpp"

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

    unsigned int create_shader(const char* vertex_path, const char* frag_path);
    unsigned int load_texture(const char* path);
    unsigned int m_shader_program;
    unsigned int m_vao, m_vbo;

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
};
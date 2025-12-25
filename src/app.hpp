#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

#include "arena.hpp"

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

    struct G {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float playerZ = 0.0f;
    } g;

    Arena m_LevelArena;
    Arena m_FrameArena;
};
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game/level1.hpp"

App::App(const std::string &title, int width, int height)
    : m_IsRunning(false) {

    m_Window = std::make_unique<Window>(title, width, height);

    g = { 0.0f, 0.0f, 0.0f };

    init();
}

App::~App() {
    m_FrameArena.destroy();
}

void App::init() {
    // Frame Arena: 1MB (Scratchpad for per-frame calculations)
    m_FrameArena.init(1 * 1024 * 1024);
    std::cout << "Initialized Frame Arena (1MB)" << std::endl;

    m_Renderer.Init();

    m_CurrentScene = std::make_unique<Level1>();
    m_CurrentScene->Init();

    m_IsRunning = true;
}

void App::run() {
    float lastFrame = 0.0f;

    while (!m_Window->ShouldClose() && m_IsRunning) {
        // --- Time Management ---
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- Memory Management ---
        // VITAL: Reset the scratchpad arena every frame.
        // This makes all "temporary" allocations from the previous frame invalid,
        // effectively "freeing" them instantly with zero cost.
        m_FrameArena.reset();

        // --- The Loop ---
        process_input(deltaTime);
        m_CurrentScene->Update(deltaTime);

        int width, height;
        glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
        m_CurrentScene->Render(m_Renderer, width, height);

        m_Window->Update();
    }
}

void App::process_input(float dt) {
    GLFWwindow* window = m_Window->GetNativeWindow();

    m_CurrentScene->ProcessInput(window, dt);

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (m_FirstMouse) {
            m_LastX = xpos;
            m_LastY = ypos;
            m_FirstMouse = false;
        }

        float xoffset = xpos - m_LastX;
        float yoffset = m_LastY - ypos; // Reversed: y-coords range from bottom to top

        m_LastX = xpos;
        m_LastY = ypos;

        // Pass to Camera (ensure you have m_Camera implemented)
        m_CurrentScene->m_Camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void App::update(float dt) {
    // Game Logic goes here
    // e.g. m_Camera->update(m_State.playerX, m_State.playerY...);
}

void App::render() {
    int width, height;
    glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
    glViewport(0, 0, width, height);
}

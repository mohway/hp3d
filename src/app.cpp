#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game/level1.hpp"
#include "engine/imgui_layer.hpp"
#include "engine/imgui/asset_browser.hpp"
#include "engine/imgui/scene_hierarchy.hpp"
#include "imgui.h"

App::App(const std::string &title, int width, int height)
    : m_IsRunning(false) {

    // Start in Fullscreen
    m_Window = std::make_unique<Window>(title, width, height, true);

    g = { 0.0f, 0.0f, 0.0f };

    init();
}

App::~App() {
    ImGuiLayer::Shutdown();
    m_FrameArena.destroy();
}

void App::init() {
    // Frame Arena: 1MB (Scratchpad for per-frame calculations)
    m_FrameArena.init(1 * 1024 * 1024);
    std::cout << "Initialized Frame Arena (1MB)" << std::endl;

    m_Renderer.Init();
    
    // Initialize ImGui
    ImGuiLayer::Init(m_Window->GetNativeWindow());

    m_CurrentScene = std::make_unique<Level1>();
    m_CurrentScene->Init();

    m_IsRunning = true;
    
    // Start with Editor hidden and cursor captured (Game Mode)
    m_EditorActive = false;
    m_Window->SetCursorCaptured(true);
}

void App::run() {
    float lastFrame = 0.0f;
    bool show_asset_window = true; // Default to open for now
    bool show_scene_hierarchy = true; // Default to open for now

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
        
        // --- ImGui Render ---
        if (m_EditorActive) {
            ImGuiLayer::Begin();
            
            // Main Menu Bar
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("View")) {
                    ImGui::MenuItem("Asset Browser", NULL, &show_asset_window);
                    ImGui::MenuItem("Scene Hierarchy", NULL, &show_scene_hierarchy);
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Metrics Window - Positioned Top Left
            ImGui::SetNextWindowPos(ImVec2(10, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(200, 70), ImGuiCond_FirstUseEver);
            ImGui::Begin("Performance Metrics");
            ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
            ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
            ImGui::End();
            
            // Asset Browser
            if (show_asset_window) {
                ImGuiWindows::ShowAssetBrowser(&show_asset_window);
            }

            // Scene Hierarchy
            if (show_scene_hierarchy) {
                ImGuiWindows::ShowSceneHierarchy(&show_scene_hierarchy, m_CurrentScene->GetObjects());
            }
            
            ImGuiLayer::End();
        }

        m_Window->Update();
    }
}

void App::process_input(float dt) {
    GLFWwindow* window = m_Window->GetNativeWindow();
    
    // Toggle Editor with F1
    static bool isF1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (!isF1Pressed) {
            m_EditorActive = !m_EditorActive;
            m_Window->SetCursorCaptured(!m_EditorActive);
            isF1Pressed = true;
        }
    } else {
        isF1Pressed = false;
    }

    // If Editor is active, ImGui handles input.
    // If Game is active, we process game input.
    if (m_EditorActive) {
        // Let ImGui handle things.
        // We might still want to process some global shortcuts here.
        return; 
    }

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
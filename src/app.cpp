#include "app.hpp"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Added: Error callback to get detailed failure messages from GLFW
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

App::App(const std::string &title, int width, int height)
    :m_Window(nullptr), m_Width(width), m_Height(height), m_IsRunning(false){

    g = { 0.0f, 0.0f, 0.0f };

    init();
}

App::~App() {
    m_LevelArena.destroy();
    m_FrameArena.destroy();

    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }

    glfwTerminate();
}

void App::init() {
    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FIX: Required for macOS to use OpenGL 3.3+ Core Profile
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_Window = glfwCreateWindow(m_Width, m_Height, "hp3d", NULL, NULL);
    if (m_Window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(m_Window);
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);

    // --- 2. Initialize GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // --- 3. Initialize Memory Arenas ---
    // Level Arena: 64MB (Huge block for Mesh, Textures, Sound that stay loaded)
    m_LevelArena.init(64 * 1024 * 1024);
    std::cout << "Initialized Level Arena (64MB)" << std::endl;

    // Frame Arena: 1MB (Scratchpad for per-frame calculations)
    m_FrameArena.init(1 * 1024 * 1024);
    std::cout << "Initialized Frame Arena (1MB)" << std::endl;

    // --- 4. Initialize Subsystems ---
    // m_Renderer = std::make_unique_ptr<Renderer>(); // TODO: Uncomment when Renderer class exists
    // m_Camera = std::make_unique_ptr<Camera>();     // TODO: Uncomment when Camera class exists

    m_IsRunning = true;
}

void App::run() {
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(m_Window) && m_IsRunning) {
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
        update(deltaTime);
        render();

        // --- Window Management ---
        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
}

void App::process_input(float dt) {
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_Window, true);

    // Example: Move player state
    // if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
    //    m_State.playerZ -= 5.0f * dt;
}

void App::update(float dt) {
    // Game Logic goes here
    // e.g. m_Camera->update(m_State.playerX, m_State.playerY...);
}

void App::render() {
    // 1. Clear Screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // "PS1 Dark Void" Color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Render Scene
    // if (m_Renderer) {
    //     m_Renderer->beginScene(m_Camera.get());
    //     m_Renderer->submit(someModel);
    //     m_Renderer->endScene();
    // }
}
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

App::App(const std::string &title, int width, int height)
    : m_IsRunning(false), m_Camera(glm::vec3(0.0f, 1.0f, 3.0f)) {

    m_Window = std::make_unique<Window>(title, width, height);

    g = { 0.0f, 0.0f, 0.0f };

    init();
}

App::~App() {
    m_LevelArena.destroy();
    m_FrameArena.destroy();
}

void App::init() {
    // --- 3. Initialize Memory Arenas ---
    // Level Arena: 64MB (Huge block for Mesh, Textures, Sound that stay loaded)
    m_LevelArena.init(64 * 1024 * 1024);
    std::cout << "Initialized Level Arena (64MB)" << std::endl;

    // Frame Arena: 1MB (Scratchpad for per-frame calculations)
    m_FrameArena.init(1 * 1024 * 1024);
    std::cout << "Initialized Frame Arena (1MB)" << std::endl;

    m_Renderer.Init();

    // --- 4. Initialize Subsystems ---
    // m_Renderer = std::make_unique_ptr<Renderer>(); // TODO: Uncomment when Renderer class exists
    // m_Camera = std::make_unique_ptr<Camera>();     // TODO: Uncomment when Camera class exists

    ResourceManager::LoadShader("../shaders/retro.vert", "../shaders/retro.frag", "retro");
    ResourceManager::LoadTexture("../textures/zwin_02.png", "zwin_floor");

    m_HarryModel = ResourceManager::LoadModel("../assets/skharrymesh.obj", "harry");

    float size = 50.0f;
    int gridX = 10;
    int gridZ = 10;
    float stepX = (size * 2) / gridX;
    float stepZ = (size * 2) / gridZ;
    float uvScale = 1.0f;

    // We need 6 vertices per square * gridX * gridZ * 5 floats per vert
    int floatCount = gridX * gridZ * 6 * 8;
    float* arenaVertices = m_LevelArena.alloc_array<float>(floatCount);
    int idx = 0;

    for (int z = 0; z < gridZ; ++z) {
        for (int x = 0; x < gridX; ++x) {
            // Calculate corners
            float x0 = -size + (x * stepX);
            float z0 = -size + (z * stepZ);
            float x1 = x0 + stepX;
            float z1 = z0 + stepZ;

            // Calculate UVs (0..10 range)
            float u0 = (float)x / gridX * uvScale;
            float v0 = (float)z / gridZ * uvScale;
            float u1 = (float)(x + 1) / gridX * uvScale;
            float v1 = (float)(z + 1) / gridZ * uvScale;

            // Triangle 1
            // P1 (x0, z0)
            arenaVertices[idx++] = x0; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z0; // Pos
            arenaVertices[idx++] = u0; arenaVertices[idx++] = v0; // Tex
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal (UP)

            // P2
            arenaVertices[idx++] = x1; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z0;
            arenaVertices[idx++] = u1; arenaVertices[idx++] = v0;
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal

            // P3
            arenaVertices[idx++] = x1; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z1;
            arenaVertices[idx++] = u1; arenaVertices[idx++] = v1;
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal

            // Triangle 2
            // P1
            arenaVertices[idx++] = x0; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z0;
            arenaVertices[idx++] = u0; arenaVertices[idx++] = v0;
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal

            // P2
            arenaVertices[idx++] = x1; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z1;
            arenaVertices[idx++] = u1; arenaVertices[idx++] = v1;
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal

            // P3
            arenaVertices[idx++] = x0; arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = z1;
            arenaVertices[idx++] = u0; arenaVertices[idx++] = v1;
            arenaVertices[idx++] = 0.0f; arenaVertices[idx++] = 1.0f; arenaVertices[idx++] = 0.0f; // Normal
        }
    }
    // Store vertex count for draw call (gridX * gridZ * 6 vertices)
    m_FloorVertexCount = gridX * gridZ * 6; // Add this member to App class!

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), arenaVertices, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // 2. TexCoord (Location 1) - Offset is 3 floats
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    // 3. Normal (Location 2) - Offset is 5 floats (3 Pos + 2 Tex)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile the screen shader
    ResourceManager::LoadShader("../shaders/screen.vert", "../shaders/screen.frag", "screen");

    glGenFramebuffers(1, &m_ShadowMapFBO);

    glGenTextures(1, &m_ShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Clamp to Border (Critical! prevents shadows repeating outside the map)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMapTexture, 0);
    glDrawBuffer(GL_NONE); // No color buffer needed
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Compile Shadow Shader
    ResourceManager::LoadShader("../shaders/shadow.vert", "../shaders/shadow.frag", "shadow");

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
        update(deltaTime);
        render();

        m_Window->Update();
    }
}

void App::process_input(float dt) {
    GLFWwindow* window = m_Window->GetNativeWindow();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        tabPressed = true;
        int mode = glfwGetInputMode(window, GLFW_CURSOR);
        glfwSetInputMode(window, GLFW_CURSOR,
            (mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        tabPressed = false;
    }

    // Camera WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(0, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(1, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(2, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(3, dt);

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
        m_Camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void App::update(float dt) {
    // Game Logic goes here
    // e.g. m_Camera->update(m_State.playerX, m_State.playerY...);
}

void App::render() {
    // =========================================================
    // 1. UPDATE STEP (Calculate Physics/Math First)
    // =========================================================

    // Calculate Dynamic Light Position (Orbiting)
    float time = (float)glfwGetTime();
    // Orbit radius 20, Height 10
    glm::vec3 lightPos = glm::vec3(sin(time) * 10.0f, 5.0f, cos(time) * 10.0f);

    // Calculate Light Matrix
    // Note: Ortho means "Sun-like" parallel shadows.
    // If you want "Torch-like" perspective shadows, switch glm::ortho to glm::perspective.
    float orthoSize = 15.0f;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 100.0f); // Increased Far plane slightly

    // Make the shadow camera look at the center of the world (0,0,0) from the lightPos
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // The "MVP" for the light
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    int width, height;
    glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
    glViewport(0, 0, width, height);

    m_Renderer.DrawFrame(m_Camera, m_HarryModel, m_vao, m_FloorVertexCount);
}

#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Added: Error callback to get detailed failure messages from GLFW
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

App::App(const std::string &title, int width, int height)
    :m_Window(nullptr), m_Width(width), m_Height(height), m_IsRunning(false), m_Camera(glm::vec3(0.0f, 1.0f, 3.0f)) {

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
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

    ResourceManager::LoadShader("../shaders/retro.vert", "../shaders/retro.frag", "retro");
    ResourceManager::LoadTexture("../textures/zwin_02.png", "zwin_floor");

    m_Model = load_model("../assets/skharrymesh.obj");

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

    // --- 5. Setup Framebuffer (The "Virtual Console") ---
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Create a texture to render into (320x240)
    glGenTextures(1, &m_TexColorBuffer);
    glBindTexture(GL_TEXTURE_2D, m_TexColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, INTERNAL_WIDTH, INTERNAL_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // CRITICAL: Use GL_NEAREST for that crunchy pixelated look.
    // GL_LINEAR would make it blurry (N64 style).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_TexColorBuffer, 0);

    // Create Renderbuffer Object (RBO) for Depth/Stencil (we need depth testing!)
    glGenRenderbuffers(1, &m_RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, INTERNAL_WIDTH, INTERNAL_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

    // Check if FBO is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind back to default

    // --- 6. Setup Screen Quad (The TV Screen) ---
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_ScreenVAO);
    glGenBuffers(1, &m_ScreenVBO);
    glBindVertexArray(m_ScreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

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

    static bool tabPressed = false;
    if (glfwGetKey(m_Window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        tabPressed = true;
        int mode = glfwGetInputMode(m_Window, GLFW_CURSOR);
        glfwSetInputMode(m_Window, GLFW_CURSOR,
            (mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
    if (glfwGetKey(m_Window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        tabPressed = false;
    }

    // Camera WASD
    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(0, dt);
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(1, dt);
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(2, dt);
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(3, dt);

    if (glfwGetInputMode(m_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        double xpos, ypos;
        glfwGetCursorPos(m_Window, &xpos, &ypos);

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


    // =========================================================
    // PASS 0: SHADOW MAP (Render depth from Light POV)
    // =========================================================
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    GLuint shadow_shader = ResourceManager::GetShader("shadow");

    // Use Shadow Shader
    glUseProgram(shadow_shader);

    // Send the matrix we calculated at the top
    glUniformMatrix4fv(glGetUniformLocation(shadow_shader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // --- DRAW SCENE (Shadow Pass) ---
    // Floor
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shadow_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_FloorVertexCount);

    // Character
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.1f));
    // model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shadow_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    for (const auto& mesh : m_Model) {
        // Note: Textures usually aren't needed for shadow maps unless you do alpha discard (transparency)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);
        glUniform1i(glGetUniformLocation(shadow_shader, "u_Texture"), 0);

        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind Shadow Map


    // =========================================================
    // PASS 1: MAIN RENDER (Render Game to 320x240 FBO)
    // =========================================================
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, INTERNAL_WIDTH, INTERNAL_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint program = ResourceManager::GetShader("retro");
    glUseProgram(program);

    // --- Send Lighting Uniforms ---
    // 1. Send the SAME Matrix used in Pass 0
    glUniformMatrix4fv(glGetUniformLocation(program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    unsigned int lightPosLoc   = glGetUniformLocation(program, "u_LightPos");
    unsigned int lightColorLoc = glGetUniformLocation(program, "u_LightColor");
    unsigned int rangeLoc      = glGetUniformLocation(program, "u_LightRange");
    unsigned int ambientLoc    = glGetUniformLocation(program, "u_AmbientColor");

    // 2. Send the SAME Position calculated at the top
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

    glUniform3f(lightColorLoc, 1.0f, 0.8f, 0.6f);    // Warm torch color
    glUniform1f(rangeLoc, 50.0f);                    // 50 unit radius
    glUniform3f(ambientLoc, 0.2f, 0.2f, 0.3f);       // Dark blue ambient

    // Set View/Proj (Camera)
    float aspectRatio = (float)INTERNAL_WIDTH / (float)INTERNAL_HEIGHT;
    glm::mat4 projection = glm::perspective(glm::radians(m_Camera.Zoom), aspectRatio, 0.1f, 1000.0f);
    glm::mat4 view = m_Camera.GetViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform2f(glGetUniformLocation(program, "u_SnapResolution"), INTERNAL_WIDTH/2.0f, INTERNAL_HEIGHT/2.0f);

    // Bind Shadow Map to Unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMapTexture);
    glUniform1i(glGetUniformLocation(program, "u_ShadowMap"), 1);

    // --- DRAW SCENE (Main Pass) ---
    // Floor
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ResourceManager::GetTexture("zwin_floor"));
    glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_FloorVertexCount);

    // Character
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.1f));
    // model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));

    for (const auto& mesh : m_Model) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);
        glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }


    // =========================================================
    // PASS 2: UPSCALE (Render Quad to Screen)
    // =========================================================
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int displayW, displayH;
    glfwGetFramebufferSize(m_Window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); // Disable depth for screen quad

    glUseProgram(ResourceManager::GetShader("screen"));
    glBindVertexArray(m_ScreenVAO);
    glBindTexture(GL_TEXTURE_2D, m_TexColorBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

App::Model App::load_model(const char* objPath) {
    Model model; // The list of sub-meshes we will return

    // 1. TinyObj Loader Variables
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Get the base directory of the OBJ so we can find textures next to it
    std::string baseDir = objPath;
    if (baseDir.find_last_of("/\\") != std::string::npos) {
        baseDir = baseDir.substr(0, baseDir.find_last_of("/\\") + 1);
    } else {
        baseDir = "";
    }

    // 2. Load the OBJ and the MTL
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath, baseDir.c_str());

    if (!warn.empty()) std::cout << "OBJ Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "OBJ Error: " << err << std::endl;
    if (!ret) return model;

    // 3. Group Geometry by Material
    // Map: Material Index -> List of Floats (Vertex Data)
    // We use a map so we can blindly throw triangles into buckets
    std::map<int, std::vector<float>> sortedGeometry;

    // Loop over all shapes (objects in the file)
    for (const auto& shape : shapes) {
        // Loop over all faces (triangles)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {

            // Get the material ID for this specific face
            int currentMaterialId = shape.mesh.material_ids[f];

            // If the mesh has no material (-1), group it into a default bucket (0)
            if (currentMaterialId < 0) currentMaterialId = 0;

            // Get the 3 vertices of this face
            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                // --- POSITIONS ---
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                // --- TEXCOORDS ---
                if (idx.texcoord_index >= 0) {
                    sortedGeometry[currentMaterialId].push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                    sortedGeometry[currentMaterialId].push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                } else {
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                }

                if (idx.normal_index >= 0) {
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 0]);
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 1]);
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 2]);
                } else {
                    // Fallback if OBJ has no normals (Up vector)
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                    sortedGeometry[currentMaterialId].push_back(1.0f);
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                }
            }
            index_offset += 3;
        }
    }

    // 4. Create a SubMesh for each material group
    for (auto& [matID, data] : sortedGeometry) {
        SubMesh subMesh = {};

        // A. Load the Texture for this group
        if (matID < materials.size() && !materials[matID].diffuse_texname.empty()) {
            std::string texPath = baseDir + materials[matID].diffuse_texname;
            // Check if we already loaded this texture? (Optimization for later)
            subMesh.textureID = ResourceManager::LoadTexture(texPath.c_str(), materials[matID].diffuse_texname);
        } else {
            // Fallback texture if none specified in MTL
            subMesh.textureID = ResourceManager::GetTexture("zwin_floor");
        }

        // B. Create VAO/VBO
        subMesh.vertexCount = data.size() / 5;

        // Copy to Arena
        float* arenaBuffer = m_LevelArena.alloc_array<float>(data.size());
        memcpy(arenaBuffer, data.data(), data.size() * sizeof(float));

        glGenVertexArrays(1, &subMesh.vao);
        glGenBuffers(1, &subMesh.vbo);
        glBindVertexArray(subMesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, subMesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), arenaBuffer, GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // 2. Tex (Location 1) - Offset 3 floats
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // 3. Normals (Location 2) - Offset 5 floats
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

        glBindVertexArray(0);

        // Add to our list
        model.push_back(subMesh);
    }

    std::cout << "Loaded Model with " << model.size() << " sub-meshes." << std::endl;
    return model;
}
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Added: Error callback to get detailed failure messages from GLFW
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

unsigned int App::load_texture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // PS1 textures didn't strictly flip, but OpenGL usually expects it.
    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // PS1 Style: Pixelated textures (Nearest Neighbor)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
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

    m_shader_program = create_shader("../shaders/retro.vert", "../shaders/retro.frag");

    m_FloorTexture = load_texture("../textures/zwin_02.png"); // Make sure to create this folder/file!
    m_Model = load_model("../assets/skharrymesh.obj");

    float rawVertices[] = {
        // Position           // TexCoord
        -5.0f, 0.0f, -5.0f,   0.0f, 0.0f,
         5.0f, 0.0f, -5.0f,   1.0f, 0.0f,
         5.0f, 0.0f,  5.0f,   1.0f, 1.0f,
        -5.0f, 0.0f, -5.0f,   0.0f, 0.0f,
         5.0f, 0.0f,  5.0f,   1.0f, 1.0f,
        -5.0f, 0.0f,  5.0f,   0.0f, 1.0f
    };

    float* arenaVertices = m_LevelArena.alloc_array<float>(15);
    memcpy(arenaVertices, rawVertices, sizeof(rawVertices));

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rawVertices), arenaVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind
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
    m_ScreenShader = create_shader("../shaders/screen.vert", "../shaders/screen.frag");

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
    // PASS 1: Render the GAME to the tiny FBO (320x240)
    // =========================================================
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, INTERNAL_WIDTH, INTERNAL_HEIGHT); // Resize viewport to 320x240

    // Clear the FBO
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST); // Enable depth testing for the 3D scene

    // --- DRAW SCENE START ---
    glUseProgram(m_shader_program);

    for (const auto& mesh : m_Model) {
        // 1. Bind THIS part's texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);

        // 2. Bind THIS part's geometry
        glBindVertexArray(mesh.vao);

        // 3. Draw
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }

    /*glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_FloorTexture); // Bind your loaded texture
    glUniform1i(glGetUniformLocation(m_shader_program, "u_Texture"), 0);*/

    // Recalculate aspect ratio for 320x240!
    float aspectRatio = (float)INTERNAL_WIDTH / (float)INTERNAL_HEIGHT;
    glm::mat4 projection = glm::perspective(glm::radians(m_Camera.Zoom), aspectRatio, 0.1f, 100.0f);

    glm::mat4 view = m_Camera.GetViewMatrix();

    unsigned int modelLoc = glGetUniformLocation(m_shader_program, "model");
    unsigned int viewLoc  = glGetUniformLocation(m_shader_program, "view");
    unsigned int projLoc  = glGetUniformLocation(m_shader_program, "projection");
    unsigned int snapLoc  = glGetUniformLocation(m_shader_program, "u_SnapResolution");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // IMPORTANT: Snap resolution should match the internal resolution height roughly
    glUniform1f(snapLoc, (float)INTERNAL_HEIGHT);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // --- DRAW SCENE END ---

    // =========================================================
    // PASS 2: Render the FBO Texture to the Screen (Upscale)
    // =========================================================
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default window
    int displayW, displayH;
    glfwGetFramebufferSize(m_Window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);

    glClear(GL_COLOR_BUFFER_BIT); // We don't need depth here, just color
    glDisable(GL_DEPTH_TEST); // Disable depth test so the quad draws over everything

    glUseProgram(m_ScreenShader);
    glBindVertexArray(m_ScreenVAO);
    glBindTexture(GL_TEXTURE_2D, m_TexColorBuffer); // Bind the low-res texture
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int App::create_shader(const char* vertexPath, const char* fragmentPath) {
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Open files (adjust paths if you run from different dirs)
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Shader Program
    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return ID;
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
            subMesh.textureID = load_texture(texPath.c_str());
        } else {
            // Fallback texture if none specified in MTL
            subMesh.textureID = m_FloorTexture;
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

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        // Add to our list
        model.push_back(subMesh);
    }

    std::cout << "Loaded Model with " << model.size() << " sub-meshes." << std::endl;
    return model;
}
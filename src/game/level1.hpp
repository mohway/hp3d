#pragma once
#include "../engine/scene.hpp"
#include "../engine/resource_manager.hpp"
#include <iostream>
#include <cmath> // For sin/cos

class Level1 : public Scene {
public:
    void Init() override {
        // 1. Initialize Memory for this Level (64MB)
        m_SceneArena.init(64 * 1024 * 1024);
        std::cout << "Level 1 Initialized (64MB Arena)" << std::endl;

        // 2. Load Global Resources
        ResourceManager::LoadShader("../shaders/retro.vert", "../shaders/retro.frag", "retro");
        ResourceManager::LoadShader("../shaders/shadow.vert", "../shaders/shadow.frag", "shadow");

        // Load Harry
        ResourceManager::LoadModel("../assets/skharrymesh.obj", "harry");
        ResourceManager::LoadTexture("../assets/skharryTex0.png", "harry_tex"); // Assuming texture name

        // Load Floor Texture
        unsigned int floorTex = ResourceManager::LoadTexture("../textures/zwin_02.png", "zwin_floor");

        // 3. GENERATE FLOOR MESH
        // We generate this procedurally and register it as a "Model" in the resource manager
        GenerateFloor(floorTex);

        // 4. CREATE GAME OBJECTS

        // --- The Floor ---
        auto* floorObj = CreateObject<MeshObject>();
        floorObj->modelResource = &ResourceManager::GetModel("floor_proc");
        floorObj->transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);

        // --- Harry Potter ---
        auto* harry = CreateObject<MeshObject>();
        harry->modelResource = &ResourceManager::GetModel("harry");
        harry->transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        harry->transform.Scale = glm::vec3(0.1f); // Adjust scale as needed
        harry->transform.Rotation = glm::vec3(-90.0f, 0.0f, 0.0f); // OBJ often needs -90 X rot

        // --- Orbiting Light ---
        m_Light = CreateObject<PointLight>();
        m_Light->transform.Position = glm::vec3(0.0f, 10.0f, 0.0f);
        m_Light->Color = glm::vec3(1.0f, 0.9f, 0.8f); // Warm sunlight
        m_Light->Intensity = 1.2f;

        // --- A Test Wall (Plane) ---
        // Just to prove the PlaneObject works (e.g., a billboard or simple wall)
        /*
        auto* wall = CreateObject<PlaneObject>();
        wall->textureID = floorTex;
        wall->transform.Position = glm::vec3(5.0f, 2.0f, 0.0f);
        wall->transform.Scale = glm::vec3(2.0f);
        */
    }

    void Update(float dt) override {
        // Animate Light Orbit
        float time = (float)glfwGetTime();
        if (m_Light) {
            m_Light->transform.Position.x = sin(time) * 15.0f;
            m_Light->transform.Position.z = cos(time) * 15.0f;
        }

        // Example: Make Harry spin slowly
        // for (auto* obj : m_Objects) {
        //     if (obj->type == ObjectType::Mesh) {
        //         obj->transform.Rotation.y += 10.0f * dt;
        //     }
        // }
    }

    void ProcessInput(GLFWwindow* window, float dt) override {
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


    }

private:
    PointLight* m_Light = nullptr; // Keep a handle to the light to animate it

    void GenerateFloor(unsigned int textureID) {
        float size = 50.0f;
        int gridX = 10;
        int gridZ = 10;
        float stepX = (size * 2) / gridX;
        float stepZ = (size * 2) / gridZ;
        float uvScale = 5.0f; // Tile the texture 5 times

        int floatCount = gridX * gridZ * 6 * 8; // 6 verts * 8 floats per vert

        // ALLOCATE FROM SCENE ARENA (Automatic cleanup on level unload!)
        float* vertices = m_SceneArena.alloc_array<float>(floatCount);
        int idx = 0;

        for (int z = 0; z < gridZ; ++z) {
            for (int x = 0; x < gridX; ++x) {
                float x0 = -size + (x * stepX);
                float z0 = -size + (z * stepZ);
                float x1 = x0 + stepX;
                float z1 = z0 + stepZ;

                float u0 = (float)x / gridX * uvScale;
                float v0 = (float)z / gridZ * uvScale;
                float u1 = (float)(x + 1) / gridX * uvScale;
                float v1 = (float)(z + 1) / gridZ * uvScale;

                // Quad made of 2 triangles
                // P1
                vertices[idx++] = x0; vertices[idx++] = 0.0f; vertices[idx++] = z0;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u0; vertices[idx++] = v0;
                // P2
                vertices[idx++] = x1; vertices[idx++] = 0.0f; vertices[idx++] = z0;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u1; vertices[idx++] = v0;
                // P3
                vertices[idx++] = x1; vertices[idx++] = 0.0f; vertices[idx++] = z1;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u1; vertices[idx++] = v1;
                // P4
                vertices[idx++] = x0; vertices[idx++] = 0.0f; vertices[idx++] = z0;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u0; vertices[idx++] = v0;
                // P5
                vertices[idx++] = x1; vertices[idx++] = 0.0f; vertices[idx++] = z1;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u1; vertices[idx++] = v1;
                // P6
                vertices[idx++] = x0; vertices[idx++] = 0.0f; vertices[idx++] = z1;
                vertices[idx++] = 0.0f; vertices[idx++] = 1.0f; vertices[idx++] = 0.0f;
                vertices[idx++] = u0; vertices[idx++] = v1;
            }
        }

        // Upload to GPU
        SubMesh floorMesh;
        floorMesh.vertexCount = gridX * gridZ * 6;
        floorMesh.textureID = textureID;

        glGenVertexArrays(1, &floorMesh.vao);
        glGenBuffers(1, &floorMesh.vbo);
        glBindVertexArray(floorMesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, floorMesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), vertices, GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);
        // Pos (0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // Normal (1) - Note: In your renderer we need to align on attribute locations!
        // Your previous App used: 0=Pos, 1=Tex, 2=Normal.
        // Standard is often: 0=Pos, 1=Normal, 2=Tex. Let's stick to your App's previous standard:
        // App Standard: 0=Pos, 1=Tex, 2=Normal

        // RE-DOING ATTRIB POINTERS TO MATCH PREVIOUS APP STRUCTURE:
        // Pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // Tex (1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Normal (2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        // Save to Resource Manager
        Model floorModel;
        floorModel.push_back(floorMesh);
        ResourceManager::Models["floor_proc"] = floorModel;
    }
};
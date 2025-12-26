#include "renderer.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h> // For glfwGetTime

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::Init() {
    InitFramebuffers();
    InitScreenQuad();

    float planeVertices[] = {
        // Pos           // Normal       // Tex
        -0.5f, 0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        -0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,

        -0.5f, 0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f, 0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);
    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // Tex is last 2
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // Norm is middle 3
}

void Renderer::Shutdown() {
    glDeleteFramebuffers(1, &m_FBO);
    glDeleteTextures(1, &m_TexColorBuffer);
    glDeleteRenderbuffers(1, &m_RBO);
    glDeleteFramebuffers(1, &m_ShadowMapFBO);
    glDeleteTextures(1, &m_ShadowMapTexture);
    glDeleteVertexArrays(1, &m_ScreenVAO);
    glDeleteBuffers(1, &m_ScreenVBO);
}

void Renderer::InitFramebuffers() {
    // --- 1. Retro FBO (320x240) ---
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    glGenTextures(1, &m_TexColorBuffer);
    glBindTexture(GL_TEXTURE_2D, m_TexColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, INTERNAL_WIDTH, INTERNAL_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_TexColorBuffer, 0);

    glGenRenderbuffers(1, &m_RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, INTERNAL_WIDTH, INTERNAL_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::RENDERER:: Framebuffer is not complete!" << std::endl;

    // --- 2. Shadow Map FBO ---
    glGenFramebuffers(1, &m_ShadowMapFBO);
    glGenTextures(1, &m_ShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::InitScreenQuad() {
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
}

// =========================================================================
// RENDER LOOP
// =========================================================================

void Renderer::RenderScene(const Camera& camera, const std::vector<GameObject*>& objects, int screenWidth, int screenHeight) {
    glm::vec3 lightPos(0, 10, 0);

    for (const auto* obj : objects) {
        if (obj->type == ObjectType::Light) {
            lightPos = obj->transform.Position;

            break; //just the first light for now TODO
        }
    }

    // 2. Calculate Matrices
    float orthoSize = 15.0f;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 100.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // 3. Execute Passes
    RenderShadowMap(lightSpaceMatrix, objects);
    RenderGeometry(camera, lightPos, lightSpaceMatrix, objects);
    RenderComposite(screenHeight, screenWidth);
}
// NOTE: Fixed overload signature here
void Renderer::RenderShadowMap(const glm::mat4& lightSpaceMatrix, const std::vector<GameObject*>& objects) {
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    unsigned int shadowShader = ResourceManager::GetShader("shadow");
    glUseProgram(shadowShader);
    glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    for (const auto& obj : objects) {
        if (!obj->visible || obj->type == ObjectType::Light) continue;

        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(obj->transform.GetMatrix()));

        if (obj->type == ObjectType::Mesh) {
            auto* mesh_object = dynamic_cast<MeshObject*>(obj);

            if (mesh_object->modelResource) {
                for (const auto& submesh : *mesh_object->modelResource) {
                    DrawMesh(submesh, shadowShader);
                }
            }
        } else if (obj->type == ObjectType::Plane) {
            glBindVertexArray(m_QuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderGeometry(const Camera& camera, const glm::vec3& lightPos, const glm::mat4& lightSpaceMatrix, const std::vector<GameObject*>& objects) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, INTERNAL_WIDTH, INTERNAL_HEIGHT);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    unsigned int program = ResourceManager::GetShader("retro");
    glUseProgram(program);

    // Light & Shadow Uniforms
    glUniformMatrix4fv(glGetUniformLocation(program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform3f(glGetUniformLocation(program, "u_LightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(program, "u_LightColor"), 1.0f, 0.8f, 0.6f);
    glUniform1f(glGetUniformLocation(program, "u_LightRange"), 50.0f);
    glUniform3f(glGetUniformLocation(program, "u_AmbientColor"), 0.2f, 0.2f, 0.3f);

    // Camera Uniforms
    float aspectRatio = (float)INTERNAL_WIDTH / (float)INTERNAL_HEIGHT;
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 1000.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform2f(glGetUniformLocation(program, "u_SnapResolution"), INTERNAL_WIDTH / 2.0f, INTERNAL_HEIGHT / 2.0f);

    // Bind Shadow Map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMapTexture);
    glUniform1i(glGetUniformLocation(program, "u_ShadowMap"), 1);

   for (auto* obj : objects) {
       if (!obj->visible) continue;

       glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(obj->transform.GetMatrix()));

       switch (obj->type) {
           case ObjectType::Mesh: {
               auto* mesh_object = dynamic_cast<MeshObject*>(obj);

               if (mesh_object->modelResource) {
                   for (const auto& submesh : *mesh_object->modelResource) {
                       glActiveTexture(GL_TEXTURE0);
                       glBindTexture(GL_TEXTURE_2D, submesh.textureID);
                       glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
                       DrawMesh(submesh, program);
                   }
               }

               break;
           }
           case ObjectType::Plane: {
               auto* plane_object = dynamic_cast<PlaneObject*>(obj);
               glActiveTexture(GL_TEXTURE0);
               glBindTexture(GL_TEXTURE_2D, plane_object->textureID);
               glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);

               glBindVertexArray(m_QuadVAO);
               glDrawArrays(GL_TRIANGLES, 0, 6);
               glBindVertexArray(0);

               break;
           }
           default: {
               std::cout << "Renderer: tried to render something besides a plane or a mesh!" << std::endl;
               break;
           };
       }
   }
}

void Renderer::RenderComposite(int screenHeight, int screenWidth) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to screen
    glViewport(0, 0, screenWidth, screenHeight);
    
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST); 

    unsigned int screenShader = ResourceManager::GetShader("screen");
    if(screenShader == 0) std::cout << "ERROR: Screen shader not found!" << std::endl;
    glUseProgram(screenShader);

    glBindVertexArray(m_ScreenVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TexColorBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::DrawMesh(const SubMesh& mesh, unsigned int shaderID) {
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    glBindVertexArray(0);
}
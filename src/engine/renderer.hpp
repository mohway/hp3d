#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "game_object.hpp"
#include "resource_manager.hpp"
#include "../camera.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Init();
    void Shutdown();

    void RenderScene(const Camera& camera,
                     const std::vector<GameObject*>& objects,
                     int screenWidth,
                     int screenHeight);

private:
    // --- Initialization Helpers ---
    void InitFramebuffers();
    void InitScreenQuad();

    void DrawQuad();

    // --- Render Passes ---
    void RenderShadowMap(const glm::mat4& lightSpaceMatrix,
                         const std::vector<GameObject*>& objects);

    void RenderGeometry(const Camera& camera,
                        const glm::vec3& lightPos,
                        const glm::mat4& lightSpaceMatrix,
                        const std::vector<GameObject*>& objects);

    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;

    void RenderComposite(int screenHeight, int screenWidth); // Upscale to screen

    // --- Helpers ---
    void DrawMesh(const SubMesh& mesh, unsigned int shaderID);

    // Settings
    const int INTERNAL_WIDTH = 320;
    const int INTERNAL_HEIGHT = 240;
    const int SHADOW_WIDTH = 2048;
    const int SHADOW_HEIGHT = 2048;

    // Render State (FBOs)
    unsigned int m_FBO;
    unsigned int m_TexColorBuffer;
    unsigned int m_RBO;

    unsigned int m_ShadowMapFBO;
    unsigned int m_ShadowMapTexture;

    // Screen Quad (for upscaling)
    unsigned int m_ScreenVAO;
    unsigned int m_ScreenVBO;
};
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

    void RenderComposite(int screenHeight, int screenWidth);

    void DrawMesh(const SubMesh& mesh, unsigned int shaderID);

    const int INTERNAL_WIDTH = 640;
    const int INTERNAL_HEIGHT = 480;
    const int SHADOW_WIDTH = 2048;
    const int SHADOW_HEIGHT = 2048;

    unsigned int m_FBO;
    unsigned int m_TexColorBuffer;
    unsigned int m_RBO;

    unsigned int m_ShadowMapFBO;
    unsigned int m_ShadowMapTexture;

    unsigned int m_ScreenVAO;
    unsigned int m_ScreenVBO;
};
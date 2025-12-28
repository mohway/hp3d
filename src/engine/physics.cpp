#include "physics.hpp"
#include "resource_manager.hpp" // To access shaders
#include <glad/glad.h>          // Needed for OpenGL commands
#include <cmath>

std::vector<PhysicsDebugDrawer::LineBatch> PhysicsDebugDrawer::m_Batches;
unsigned int PhysicsDebugDrawer::m_VAO = 0;
unsigned int PhysicsDebugDrawer::m_VBO = 0;

void PhysicsDebugDrawer::Init() {
    if (m_VAO != 0) return; // Already initialized

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    
    // Position attribute (Location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void PhysicsDebugDrawer::DrawLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color) {
    // Find or create a batch for this color
    auto it = std::find_if(m_Batches.begin(), m_Batches.end(),
        [&](const LineBatch& b) { return b.color == color; });

    if (it == m_Batches.end()) {
        m_Batches.push_back({color, {}});
        it = m_Batches.end() - 1;
    }

    std::vector<float>& v = it->vertices;
    v.push_back(p1.x); v.push_back(p1.y); v.push_back(p1.z);
    v.push_back(p2.x); v.push_back(p2.y); v.push_back(p2.z);
}

void PhysicsDebugDrawer::DrawTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& color) {
    DrawLine(p1, p2, color);
    DrawLine(p2, p3, color);
    DrawLine(p3, p1, color);
}

void PhysicsDebugDrawer::DrawAABB(const AABB& box, glm::vec3 color) {
    // Find or create a batch for this color
    auto it = std::find_if(m_Batches.begin(), m_Batches.end(), 
        [&](const LineBatch& b) { return b.color == color; });
    
    if (it == m_Batches.end()) {
        m_Batches.push_back({color, {}});
        it = m_Batches.end() - 1;
    }

    std::vector<float>& v = it->vertices;
    float minX = box.min.x, minY = box.min.y, minZ = box.min.z;
    float maxX = box.max.x, maxY = box.max.y, maxZ = box.max.z;

    // Helper lambda to add a line
    auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2) {
        v.push_back(x1); v.push_back(y1); v.push_back(z1);
        v.push_back(x2); v.push_back(y2); v.push_back(z2);
    };

    // Bottom Face
    addLine(minX, minY, minZ, maxX, minY, minZ);
    addLine(maxX, minY, minZ, maxX, minY, maxZ);
    addLine(maxX, minY, maxZ, minX, minY, maxZ);
    addLine(minX, minY, maxZ, minX, minY, minZ);

    // Top Face
    addLine(minX, maxY, minZ, maxX, maxY, minZ);
    addLine(maxX, maxY, minZ, maxX, maxY, maxZ);
    addLine(maxX, maxY, maxZ, minX, maxY, maxZ);
    addLine(minX, maxY, maxZ, minX, maxY, minZ);

    // Verticals
    addLine(minX, minY, minZ, minX, maxY, minZ);
    addLine(maxX, minY, minZ, maxX, maxY, minZ);
    addLine(maxX, minY, maxZ, maxX, maxY, maxZ);
    addLine(minX, minY, maxZ, minX, maxY, maxZ);
}

void PhysicsDebugDrawer::DrawCylinder(const Cylinder& cyl, glm::vec3 color) {
    // Draw a simplified wireframe for the cylinder (top/bottom crosses + verticals)
    // You can expand this to draw circles later if you want.
    AABB box;
    box.min = cyl.position - glm::vec3(cyl.radius, 0.0f, cyl.radius);
    box.max = cyl.position + glm::vec3(cyl.radius, cyl.height, cyl.radius);
    
    // Reuse AABB draw for the bounding box
    DrawAABB(box, color);
}

void PhysicsDebugDrawer::Render(const glm::mat4& view, const glm::mat4& projection) {
    if (m_Batches.empty()) return;

    // Use your existing debug shader
    int shader = ResourceManager::GetShader("debug");
    glUseProgram(shader);

    // Set View/Proj once
    // Note: Assuming your ShaderProgram class has SetMatrix4 methods. 
    // If not, use glUniformMatrix4fv directly.
    unsigned int viewLoc = glGetUniformLocation(shader, "view");
    unsigned int projLoc = glGetUniformLocation(shader, "projection");
    unsigned int modelLoc = glGetUniformLocation(shader, "model");
    unsigned int colorLoc = glGetUniformLocation(shader, "color");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    // Identity Model matrix (since our lines are already in World Space)
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Draw each batch
    for (const auto& batch : m_Batches) {
        if (batch.vertices.empty()) continue;

        // Upload
        glBufferData(GL_ARRAY_BUFFER, batch.vertices.size() * sizeof(float), batch.vertices.data(), GL_DYNAMIC_DRAW);

        // Set Color
        glUniform3f(colorLoc, batch.color.x, batch.color.y, batch.color.z);

        // Draw
        glDrawArrays(GL_LINES, 0, batch.vertices.size() / 3);
    }

    glBindVertexArray(0);
    
    // Clear batches for next frame
    m_Batches.clear();
}
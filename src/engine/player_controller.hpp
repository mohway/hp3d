#pragma once
#include "game_object.hpp"
#include "../camera.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtx/vector_angle.hpp> // Needed for angle calculations

class PlayerController {
public:
    PlayerController(GameObject* player, Camera* camera)
        : m_Player(player), m_Camera(camera) {

        m_Player->hasCollision = true;
        m_Player->collisionRadius = 0.3f;
        m_Player->collisionHeight = 1.8f;
    }

    void Update(GLFWwindow* window, float dt, const std::vector<GameObject*>& sceneObjects) {
        if (!m_Player || !m_Camera) return;

        if (!m_IsGrounded) m_Velocity.y += m_Gravity * dt;

        if (m_IsGrounded && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_Velocity.y = m_JumpForce;
            m_IsGrounded = false;
        }

        // 1. Get Input
        float moveX = 0.0f;
        float moveZ = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveZ =  1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveZ = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveX = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveX =  1.0f;

        // 2. Calculate Direction relative to Camera
        // We only care about the XZ plane (flat ground), so we zero out the Y component
        glm::vec3 camForward = m_Camera->Front;
        camForward.y = 0.0f;
        camForward = glm::normalize(camForward);

        glm::vec3 camRight = m_Camera->Right;
        camRight.y = 0.0f;
        camRight = glm::normalize(camRight);

        // This is the desired direction in World Space
        glm::vec3 moveDir = (camForward * moveZ) + (camRight * moveX);
        if (glm::length(moveDir) > 0.0f)
            moveDir = glm::normalize(moveDir);

        // 3. Move the Player
        // POSITION BASED m_Player->transform.Position += moveDir * m_Speed * dt;

        m_Velocity.x = moveDir.x * m_Speed;
        m_Velocity.z = moveDir.z * m_Speed;

        m_Player->transform.Position += m_Velocity * dt;

        m_IsGrounded = false;

        for (int i = 0; i < 3; i++) {
            if (const bool hitSomething = ResolveCollisions(sceneObjects); !hitSomething) break;
        }

        if (m_Player->transform.Position.y < 0.0f) {
            m_Player->transform.Position.y = 0.0f;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }

        // 4. Rotate Player to face direction
        // Calculate the angle in degrees
        if (glm::length(moveDir) > 0.1f) {
            float targetAngle = glm::degrees(atan2(moveDir.x, moveDir.z));
            targetAngle += 90.0f; // Correction for model orientation

            float currentAngle = m_Player->transform.Rotation.y;
            float angleDiff = targetAngle - currentAngle;
            while (angleDiff > 180.0f)  angleDiff -= 360.0f;
            while (angleDiff < -180.0f) angleDiff += 360.0f;
            m_Player->transform.Rotation.y += angleDiff * 10.0f * dt;
        }
    }

private:
    bool ResolveCollisions(const std::vector<GameObject*>& objects) {
        bool hit = false;

        Cylinder harryCyl;
        harryCyl.position = m_Player->transform.Position;
        harryCyl.radius = m_Player->collisionRadius;
        harryCyl.height = m_Player->collisionHeight;

        for (const auto* obj : objects) {
            if (obj == m_Player || !obj->hasCollision) continue;

            glm::vec3 pushVec(0.0f);

            // Note: UpdateSelfAndChild MUST be called on objects before this to ensure obj->collider is valid
            if (CheckCylinderAABB(harryCyl, obj->collider, pushVec)) {
                hit = true;

                // 1. Push Harry out of the wall
                m_Player->transform.Position += pushVec;
                harryCyl.position += pushVec; // Update temp for next check

                // 2. Slide Velocity (Project velocity onto the wall plane)
                // Normalize the push vector to get the Wall Normal
                if (glm::length(pushVec) > 0.0001f) {
                    glm::vec3 normal = glm::normalize(pushVec);

                    // Remove the part of velocity that goes INTO the wall
                    // Formula: V_new = V_old - (V_old . Normal) * Normal
                    float dotProd = glm::dot(m_Velocity, normal);

                    // Only slide if moving INTO the wall
                    if (dotProd < 0.0f) {
                        m_Velocity -= normal * dotProd;
                    }

                    // Check if we hit a floor/ceiling
                    if (normal.y > 0.7f) {
                        m_IsGrounded = true;
                        m_Velocity.y = 0;
                    }
                }
            }
        }
        return hit;
    }

    GameObject* m_Player;
    Camera* m_Camera;

    float m_Speed = 12.0f;      // Units per second
    float m_TurnSpeed = 10.0f; // Speed of turning
    float m_RotationOffset = 90.0f;

    glm::vec3 m_Velocity = glm::vec3(0.0f);
    float m_Gravity = -35.0f;
    float m_JumpForce = 16.0f;
    bool m_IsGrounded = false;
};
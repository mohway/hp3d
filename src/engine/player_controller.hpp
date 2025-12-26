#pragma once
#include "game_object.hpp"
#include "../camera.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtx/vector_angle.hpp> // Needed for angle calculations

class PlayerController {
public:
    PlayerController(GameObject* player, Camera* camera)
        : m_Player(player), m_Camera(camera) {}

    void Update(GLFWwindow* window, float dt) {
        if (!m_Player || !m_Camera) return;

        // 1. Get Input
        float moveX = 0.0f;
        float moveZ = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveZ =  1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveZ = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveX = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveX =  1.0f;

        // If no input, stop (and maybe play idle animation later)
        if (moveX == 0.0f && moveZ == 0.0f) return;

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
        m_Player->transform.Position += moveDir * m_Speed * dt;

        // 4. Rotate Player to face direction
        // Calculate the angle in degrees
        float targetAngle = glm::degrees(atan2(moveDir.x, moveDir.z));

        targetAngle += m_RotationOffset;

        // Smooth Rotation (Linear Interpolation for angles)
        // Note: For a real game, you'd use Quaternions (slerp), but this is fine for now.
        float currentAngle = m_Player->transform.Rotation.y;
        float angleDiff = targetAngle - currentAngle;

        // Handle wrap-around (e.g. going from 350 to 10 degrees)
        while (angleDiff > 180.0f)  angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        m_Player->transform.Rotation.y += angleDiff * m_TurnSpeed * dt;
    }

private:
    GameObject* m_Player;
    Camera* m_Camera;

    float m_Speed = 12.0f;      // Units per second
    float m_TurnSpeed = 10.0f; // Speed of turning
    float m_RotationOffset = 90.0f;
};
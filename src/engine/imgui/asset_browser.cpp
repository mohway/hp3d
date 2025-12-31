#include "asset_browser.hpp"
#include "../resource_manager.hpp"
#include "../imgui_layer.hpp" // For any shared imgui utils if needed, or just imgui.h

#include <glad/glad.h>
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace {
constexpr int kPreviewMinSize = 128;
constexpr int kPreviewMaxSize = 512;

struct PreviewTarget {
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int depthRbo = 0;
    int size = 0;
};

PreviewTarget g_PreviewTarget;
std::string g_PreviewModelKey;
glm::vec3 g_PreviewCameraRight(1.0f, 0.0f, 0.0f);
glm::vec3 g_PreviewCameraUp(0.0f, 1.0f, 0.0f);

void DestroyPreviewTarget() {
    if (g_PreviewTarget.depthRbo != 0) {
        glDeleteRenderbuffers(1, &g_PreviewTarget.depthRbo);
        g_PreviewTarget.depthRbo = 0;
    }
    if (g_PreviewTarget.colorTexture != 0) {
        glDeleteTextures(1, &g_PreviewTarget.colorTexture);
        g_PreviewTarget.colorTexture = 0;
    }
    if (g_PreviewTarget.fbo != 0) {
        glDeleteFramebuffers(1, &g_PreviewTarget.fbo);
        g_PreviewTarget.fbo = 0;
    }
    g_PreviewTarget.size = 0;
}

void EnsurePreviewTarget(int requestedSize) {
    int clampedSize = std::max(kPreviewMinSize, std::min(kPreviewMaxSize, requestedSize));
    if (g_PreviewTarget.fbo != 0 && g_PreviewTarget.size == clampedSize) {
        return;
    }

    DestroyPreviewTarget();
    g_PreviewTarget.size = clampedSize;

    glGenFramebuffers(1, &g_PreviewTarget.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_PreviewTarget.fbo);

    glGenTextures(1, &g_PreviewTarget.colorTexture);
    glBindTexture(GL_TEXTURE_2D, g_PreviewTarget.colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, clampedSize, clampedSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_PreviewTarget.colorTexture, 0);

    glGenRenderbuffers(1, &g_PreviewTarget.depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, g_PreviewTarget.depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, clampedSize, clampedSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_PreviewTarget.depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::IMGUI::Preview framebuffer incomplete." << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int EnsurePreviewShader() {
    auto it = ResourceManager::Shaders.find("preview");
    if (it == ResourceManager::Shaders.end() || it->second.ID == 0) {
        return ResourceManager::LoadShader("../shaders/preview.vert", "../shaders/preview.frag", "preview");
    }
    return it->second.ID;
}

void RenderModelPreview(const Model& model, int renderSize, unsigned int shader, const glm::quat& rotation) {
    if (shader == 0) {
        return;
    }

    EnsurePreviewTarget(renderSize);

    GLint prevFbo = 0;
    GLint prevViewport[4] = {0, 0, 0, 0};
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, g_PreviewTarget.fbo);
    glViewport(0, 0, g_PreviewTarget.size, g_PreviewTarget.size);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);

    Bounds bounds = ComputeModelBounds(model);
    glm::vec3 center = (bounds.min + bounds.max) * 0.5f;
    glm::vec3 extents = bounds.max - bounds.min;
    float maxExtent = std::max(extents.x, std::max(extents.y, extents.z));
    if (maxExtent < 0.001f) {
        maxExtent = 1.0f;
    }

    float distance = maxExtent * 2.2f;
    glm::vec3 eye = center + glm::vec3(distance, distance * 0.6f, distance);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, worldUp);
    glm::mat4 projection = glm::perspective(glm::radians(35.0f), 1.0f, 0.01f, 1000.0f);
    glm::vec3 forward = glm::normalize(center - eye);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    g_PreviewCameraRight = right;
    g_PreviewCameraUp = worldUp;

    glm::quat baseRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat finalRotation = rotation * baseRotation;

    glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), center)
                         * glm::mat4_cast(finalRotation)
                         * glm::translate(glm::mat4(1.0f), -center);

    glUniformMatrix4fv(ResourceManager::GetUniformLocation("preview", "u_View"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(ResourceManager::GetUniformLocation("preview", "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(ResourceManager::GetUniformLocation("preview", "u_Model"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3f(ResourceManager::GetUniformLocation("preview", "u_LightDir"), -0.5f, -1.0f, -0.3f);
    glUniform3f(ResourceManager::GetUniformLocation("preview", "u_Color"), 0.85f, 0.85f, 0.85f);
    glUniform1i(ResourceManager::GetUniformLocation("preview", "u_Texture"), 0);

    for (const auto& submesh : model) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, submesh.textureID);
        glUniform1i(ResourceManager::GetUniformLocation("preview", "u_UseTexture"), submesh.textureID != 0 ? 1 : 0);

        glBindVertexArray(submesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, submesh.vertexCount);
    }
    glBindVertexArray(0);

    if (prevDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (prevCullFace) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
} // namespace

namespace ImGuiWindows {

void ShowAssetBrowser(bool* p_open) {
    static fs::path s_AssetPath = "../assets";
    static fs::path s_CurrentPath = "../assets";
    static fs::path s_SelectedPath;
    static unsigned int s_PreviewTextureID = 0;
    static std::string s_PreviewExtension = "";
    static fs::path s_LastPreviewPath;
    static glm::quat s_PreviewRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    static glm::quat s_PreviewDragStartRotation = s_PreviewRotation;
    static bool s_PreviewDragging = false;

    // Ensure the assets directory exists
    if (!fs::exists(s_AssetPath)) {
        ImGui::Begin("Asset Browser", p_open);
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: 'assets' directory not found!");
        ImGui::End();
        return;
    }

    // Set initial position and size
    ImGui::SetNextWindowPos(ImVec2(10, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Asset Browser", p_open)) {
        ImGui::End();
        return;
    }

    // Layout: Split into two columns (File List | Preview)
    ImGui::Columns(2, "AssetBrowserColumns", true);

    // --- Left Column: File Navigation ---

    // Navigation Bar
    if (s_CurrentPath != s_AssetPath) {
        if (ImGui::Button("<- Back")) {
            s_CurrentPath = s_CurrentPath.parent_path();
            // Clear selection when changing directory
            s_SelectedPath.clear();
            g_PreviewModelKey.clear();
            if (s_PreviewTextureID != 0) {
                ResourceManager::DeleteTexture("AssetBrowserPreview");
                s_PreviewTextureID = 0;
            }
        }
        ImGui::SameLine();
    }
    ImGui::Text("Path: %s", s_CurrentPath.string().c_str());
    ImGui::Separator();

    // Content Area
    ImGui::BeginChild("FileList");

    try {
        for (const auto& entry : fs::directory_iterator(s_CurrentPath)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            // Filter logic
            bool isDir = entry.is_directory();
            bool isRelevantFile = false;
            std::string ext = "";

            if (!isDir) {
                ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".png" || ext == ".jpg" || ext == ".obj") {
                    isRelevantFile = true;
                }
            }

            if (isDir || isRelevantFile) {
                ImGui::PushID(filename.c_str());

                if (isDir) {
                    if (ImGui::Button(("[" + filename + "]").c_str())) {
                        s_CurrentPath /= path.filename();
                        s_SelectedPath.clear();
                        g_PreviewModelKey.clear();
                        if (s_PreviewTextureID != 0) {
                            ResourceManager::DeleteTexture("AssetBrowserPreview");
                            s_PreviewTextureID = 0;
                        }
                    }
                } else {
                    // Selectable item
                    bool isSelected = (s_SelectedPath == path);
                    if (ImGui::Selectable(filename.c_str(), isSelected)) {
                        s_SelectedPath = path;
                        s_PreviewExtension = ext;
                        s_LastPreviewPath.clear();
                        s_PreviewRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                        s_PreviewDragStartRotation = s_PreviewRotation;
                        s_PreviewDragging = false;

                        // Load preview if it's a texture
                        if (ext == ".png" || ext == ".jpg") {
                            // Clean up old preview
                            if (s_PreviewTextureID != 0) {
                                ResourceManager::DeleteTexture("AssetBrowserPreview");
                            }
                            // Load new preview
                            s_PreviewTextureID = ResourceManager::LoadTexture(path.string().c_str(), "AssetBrowserPreview");
                            g_PreviewModelKey.clear();
                        } else {
                            // Not a texture, clear preview
                             if (s_PreviewTextureID != 0) {
                                ResourceManager::DeleteTexture("AssetBrowserPreview");
                                s_PreviewTextureID = 0;
                            }
                            if (ext == ".obj") {
                                g_PreviewModelKey = path.string();
                                if (ResourceManager::Models.find(g_PreviewModelKey) == ResourceManager::Models.end()) {
                                    ResourceManager::LoadModel(path.string().c_str(), g_PreviewModelKey);
                                }
                            } else {
                                g_PreviewModelKey.clear();
                            }
                        }
                    }

                    // Drag and drop source
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                        std::string relativePath = fs::relative(path, s_AssetPath).string();
                        ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", relativePath.c_str(), relativePath.size() + 1);
                        ImGui::Text("%s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                ImGui::PopID();
            }
        }
    } catch (const fs::filesystem_error& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error accessing directory: %s", e.what());
    }
    ImGui::EndChild(); // FileList

    ImGui::NextColumn();

    // --- Right Column: Preview ---
    ImGui::Text("Preview");
    ImGui::Separator();

    if (!s_SelectedPath.empty()) {
        ImGui::TextWrapped("File: %s", s_SelectedPath.filename().string().c_str());

        if (s_PreviewTextureID != 0) {
            // Display Texture
            // Get texture dimensions to keep aspect ratio
            // We can cheat and use a fixed size for now or query texture size if we had it stored easily
            // ResourceManager::GetTexture returns ID only.
            // But we can look it up in the map if we really wanted dimensions.
            // For now, let's just show it in a fixed box.

            float availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::Image((void*)(intptr_t)s_PreviewTextureID, ImVec2(availWidth, availWidth));
        } else if (s_PreviewExtension == ".obj") {
            if (g_PreviewModelKey.empty()) {
                ImGui::Text("Model preview not loaded.");
            } else {
                auto modelIt = ResourceManager::Models.find(g_PreviewModelKey);
                unsigned int previewShader = EnsurePreviewShader();
                float availWidth = ImGui::GetContentRegionAvail().x;
                float displaySize = std::max(128.0f, std::min(availWidth, 512.0f));
                int renderSize = static_cast<int>(displaySize);

                if (modelIt != ResourceManager::Models.end() && previewShader != 0) {
                    if (s_SelectedPath != s_LastPreviewPath) {
                        s_LastPreviewPath = s_SelectedPath;
                        s_PreviewRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                        s_PreviewDragStartRotation = s_PreviewRotation;
                        s_PreviewDragging = false;
                    }

                    RenderModelPreview(modelIt->second, renderSize, previewShader, s_PreviewRotation);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::ImageButton("##ModelPreview",
                                       (void*)(intptr_t)g_PreviewTarget.colorTexture,
                                       ImVec2(displaySize, displaySize),
                                       ImVec2(0, 0),
                                       ImVec2(1, 1),
                                       ImVec4(0, 0, 0, 0),
                                       ImVec4(1, 1, 1, 1));
                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();

                    if (ImGui::IsItemActivated()) {
                        s_PreviewDragging = true;
                        s_PreviewDragStartRotation = s_PreviewRotation;
                        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                    }
                    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        s_PreviewDragging = false;
                    }
                    if (s_PreviewDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                        float yaw = delta.x * 0.01f;
                        float pitch = -delta.y * 0.01f;
                        pitch = std::max(-1.5f, std::min(1.5f, pitch));

                        glm::quat yawQuat = glm::angleAxis(yaw, g_PreviewCameraUp);
                        glm::quat pitchQuat = glm::angleAxis(pitch, g_PreviewCameraRight);
                        s_PreviewRotation = yawQuat * pitchQuat * s_PreviewDragStartRotation;
                    }
                } else {
                    ImGui::Text("Model preview unavailable.");
                }
            }
        } else {
            ImGui::Text("No preview available.");
        }
    } else {
        ImGui::Text("No file selected.");
    }

    ImGui::Columns(1);
    ImGui::End();
}

} // namespace ImGuiWindows

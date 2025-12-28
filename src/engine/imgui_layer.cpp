#include "imgui_layer.hpp"
#include "resource_manager.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

void ImGuiLayer::Init(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void ImGuiLayer::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::ShowAssetBrowser(bool* p_open) {
    static fs::path s_AssetPath = "../assets";
    static fs::path s_CurrentPath = "../assets";
    static fs::path s_SelectedPath;
    static unsigned int s_PreviewTextureID = 0;
    static std::string s_PreviewExtension = "";

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
                        
                        // Load preview if it's a texture
                        if (ext == ".png" || ext == ".jpg") {
                            // Clean up old preview
                            if (s_PreviewTextureID != 0) {
                                ResourceManager::DeleteTexture("AssetBrowserPreview");
                            }
                            // Load new preview
                            s_PreviewTextureID = ResourceManager::LoadTexture(path.string().c_str(), "AssetBrowserPreview");
                        } else {
                            // Not a texture, clear preview
                             if (s_PreviewTextureID != 0) {
                                ResourceManager::DeleteTexture("AssetBrowserPreview");
                                s_PreviewTextureID = 0;
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
            ImGui::Text("Model Preview not available yet.");
        } else {
            ImGui::Text("No preview available.");
        }
    } else {
        ImGui::Text("No file selected.");
    }

    ImGui::Columns(1);
    ImGui::End();
}

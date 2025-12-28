#include "scene_hierarchy.hpp"
#include "../game_object.hpp"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace ImGuiWindows {

void ShowSceneHierarchy(bool* p_open, std::vector<GameObject*>& objects) {
    static GameObject* s_SelectedObject = nullptr;

    if (!ImGui::Begin("Scene Hierarchy", p_open)) {
        ImGui::End();
        return;
    }

    ImGui::Columns(2, "SceneHierarchyColumns", true);

    // --- Left Column: Object List ---
    ImGui::BeginChild("ObjectList");
    for (size_t i = 0; i < objects.size(); ++i) {
        GameObject* obj = objects[i];
        std::string name = "GameObject " + std::to_string(i);

        // Determine name based on type
        switch (obj->type) {
            case ObjectType::Mesh: name = "Mesh " + std::to_string(i); break;
            case ObjectType::Light: name = "Light " + std::to_string(i); break;
            case ObjectType::Plane: name = "Plane " + std::to_string(i); break;
            default: break;
        }

        if (ImGui::Selectable(name.c_str(), s_SelectedObject == obj)) {
            s_SelectedObject = obj;
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // --- Right Column: Object Details ---
    ImGui::Text("Inspector");
    ImGui::Separator();

    if (s_SelectedObject) {
        // Transform
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", glm::value_ptr(s_SelectedObject->transform.Position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(s_SelectedObject->transform.Rotation), 1.0f);
            ImGui::DragFloat3("Scale", glm::value_ptr(s_SelectedObject->transform.Scale), 0.1f);
        }

        // Type Specific Info
        ImGui::Separator();
        ImGui::Text("Type: %s", ObjectTypeToString(s_SelectedObject->type));

        if (s_SelectedObject->type == ObjectType::Light) {
            PointLight* light = static_cast<PointLight*>(s_SelectedObject);
            ImGui::ColorEdit3("Color", glm::value_ptr(light->Color));
            ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Radius", &light->Radius, 0.5f, 0.0f, 500.0f);
        }

        // Visibility
        ImGui::Checkbox("Visible", &s_SelectedObject->visible);
    } else {
        ImGui::Text("No object selected.");
    }

    ImGui::Columns(1);
    ImGui::End();
}

} // namespace ImGuiWindows

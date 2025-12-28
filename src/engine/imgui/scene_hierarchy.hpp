#pragma once

#include <vector>

struct GameObject;

namespace ImGuiWindows {
    void ShowSceneHierarchy(bool* p_open, std::vector<GameObject*>& objects);
}

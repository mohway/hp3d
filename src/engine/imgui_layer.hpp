#pragma once

#include <vector>

struct GLFWwindow;
struct GameObject;

class ImGuiLayer {
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();
    
    static void Begin();
    static void End();
};

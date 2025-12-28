#pragma once

struct GLFWwindow;

class ImGuiLayer {
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();
    
    static void Begin();
    static void End();

    // New function for the Asset Browser
    static void ShowAssetBrowser(bool* p_open);
};

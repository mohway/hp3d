#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    // The main loop checks this
    bool ShouldClose() const;

    // Swaps buffers and polls events
    void Update();

    // Getters for App to use
    GLFWwindow* GetNativeWindow() const { return m_Window; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    // Helpers
    void SetCursorCaptured(bool captured);

private:
    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    std::string m_Title;

    void Init();
    void Shutdown();
};
#include "window.hpp"

// Static callback to handle resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Static callback for errors
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

Window::Window(const std::string& title, int width, int height, bool fullscreen)
    : m_Title(title), m_Width(width), m_Height(height), m_Fullscreen(fullscreen), m_Window(nullptr) {
    Init();
}

Window::~Window() {
    Shutdown();
}

void Window::Init() {
    // 1. Init GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. Create Window
    GLFWmonitor* monitor = nullptr;
    if (m_Fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        m_Width = mode->width;
        m_Height = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        monitor = nullptr; // Borderless windowed fullscreen (not exclusive fullscreen)
    }

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), monitor, NULL);
    if (m_Window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    if (m_Fullscreen) {
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        int xpos = 0;
        int ypos = 0;
        if (primary) {
            glfwGetMonitorPos(primary, &xpos, &ypos);
        }
        glfwSetWindowPos(m_Window, xpos, ypos);
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);

    // 3. Init GLAD (Context is now valid)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // Default State
    SetCursorCaptured(true);
}

void Window::Update() {
    glfwSwapBuffers(m_Window);
    glfwPollEvents();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::SetCursorCaptured(bool captured) {
    if (captured) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Window::Shutdown() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

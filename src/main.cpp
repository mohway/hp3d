#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "app.hpp"

int main() {
    App app("hp3d", 800, 600);
    app.run();
    return 0;
}

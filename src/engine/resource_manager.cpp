
#include "resource_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::map<std::string, Texture2D> ResourceManager::Textures;
std::map<std::string, ShaderProgram> ResourceManager::Shaders;

unsigned int ResourceManager::LoadTexture(const char *file, const std::string& name) {
    if (Textures.contains(name)) {
        return Textures[name].ID;
    }

    const Texture2D texture = loadTextureFromFile(file);

    Textures[name] = texture;

    return texture.ID;
}

unsigned int ResourceManager::GetTexture(const std::string& name) {
    return Textures[name].ID;
}

unsigned int ResourceManager::LoadShader(const char *vShaderFile, const char *fShaderFile, const std::string& name) {
    if (Shaders.contains(name)) {
        return Shaders[name].ID;
    }

    const ShaderProgram shader_program = loadShaderFromFile(vShaderFile, fShaderFile);

    Shaders[name] = shader_program;

    return shader_program.ID;
}

unsigned int ResourceManager::GetShader(const std::string& name) {
    return Shaders[name].ID;
}

void ResourceManager::Clear() {
    for (const auto& iter : Textures) glDeleteTextures(1, &iter.second.ID);
    for (const auto& iter : Shaders) glDeleteProgram(iter.second.ID);
}

Texture2D ResourceManager::loadTextureFromFile(const char *file) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // PS1 textures didn't strictly flip, but OpenGL usually expects it.
    stbi_set_flip_vertically_on_load(true);

    if (unsigned char *data = stbi_load(file, &width, &height, &nrComponents, 0)) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // PS1 Style: Pixelated textures (Nearest Neighbor)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << file << std::endl;
        stbi_image_free(data);
    }

    return { .ID = textureID, .width = width, .height = height };
}

ShaderProgram ResourceManager::loadShaderFromFile(const char *vertexPath, const char *fragmentPath) {
     // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Open files (adjust paths if you run from different dirs)
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Shader Program
    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return { .ID = ID };
}

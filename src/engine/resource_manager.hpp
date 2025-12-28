#pragma once

#include <map>
#include <string>
#include <glad/glad.h>
#include <vector>

// Forward decls
struct SubMesh {
    unsigned int vao;
    unsigned int vbo;
    unsigned int textureID;
    int vertexCount;
};

// A "Model" is a collection of SubMeshes
using Model = std::vector<SubMesh>;

struct Texture2D {
    unsigned int ID;
    int width, height;
};

struct ShaderProgram {
    unsigned int ID;
    std::map<std::string, int> uniformCache; // Cache for uniform locations
    void use() { glUseProgram(ID); }
};

class ResourceManager {
public:
    static std::map<std::string, Texture2D>     Textures;
    static std::map<std::string, ShaderProgram> Shaders;
    static std::map<std::string, Model>         Models;

    static unsigned int LoadTexture(const char* file, const std::string& name);
    static unsigned int GetTexture(const std::string& name);
    static void DeleteTexture(const std::string& name);

    static unsigned int LoadShader(const char *vShaderFile, const char *fShaderFile, const std::string &name);
    static unsigned int GetShader(const std::string& name);
    
    // New helper to get cached uniform location
    static int GetUniformLocation(const std::string& shaderName, const std::string& uniformName);

    static Model& LoadModel(const char* file, std::string name);
    static Model& GetModel(std::string name);

    static void Clear();

private:
    ResourceManager() {}

    static Texture2D loadTextureFromFile(const char* file);
    static ShaderProgram loadShaderFromFile(const char* vShaderFile, const char* fShaderFile);
    static Model loadModelFromFile(const char* file);
};

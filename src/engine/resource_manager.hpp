#pragma once

#include <map>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Forward decls
struct SubMesh {
    unsigned int vao;
    unsigned int vbo;
    unsigned int textureID;
    int vertexCount;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

// A "Model" is a collection of SubMeshes
using Model = std::vector<SubMesh>;


struct Bounds {
    glm::vec3 min;
    glm::vec3 max;
    bool valid = false;
};

static Bounds ComputeModelBounds(const Model& model) {
    Bounds bounds;
    for (const auto& submesh : model) {
        if (!bounds.valid) {
            bounds.min = submesh.boundsMin;
            bounds.max = submesh.boundsMax;
            bounds.valid = true;
            continue;
        }

        bounds.min.x = std::min(bounds.min.x, submesh.boundsMin.x);
        bounds.min.y = std::min(bounds.min.y, submesh.boundsMin.y);
        bounds.min.z = std::min(bounds.min.z, submesh.boundsMin.z);
        bounds.max.x = std::max(bounds.max.x, submesh.boundsMax.x);
        bounds.max.y = std::max(bounds.max.y, submesh.boundsMax.y);
        bounds.max.z = std::max(bounds.max.z, submesh.boundsMax.z);
    }

    if (!bounds.valid) {
        bounds.min = glm::vec3(-0.5f);
        bounds.max = glm::vec3(0.5f);
        bounds.valid = true;
    }

    return bounds;
}

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

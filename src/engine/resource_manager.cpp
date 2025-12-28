#include "resource_manager.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "tiny_obj_loader.h"

// Instantiate static variables
std::map<std::string, Texture2D> ResourceManager::Textures;
std::map<std::string, ShaderProgram> ResourceManager::Shaders;
std::map<std::string, Model> ResourceManager::Models;

namespace {
std::string ResolvePath(const char* file) {
    std::filesystem::path path(file);
    if (path.is_absolute()) {
        return path.string();
    }

    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path candidate = cwd / path;
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
        return candidate.string();
    }

    std::string raw = path.generic_string();
    std::string trimmed = raw;
    while (trimmed.rfind("../", 0) == 0) {
        trimmed = trimmed.substr(3);
        candidate = cwd / trimmed;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate.string();
        }
    }

    return path.string();
}

Texture2D CreateFallbackTexture() {
    Texture2D texture{};
    unsigned char pixel[4] = {255, 0, 255, 255};

    glGenTextures(1, &texture.ID);
    glBindTexture(GL_TEXTURE_2D, texture.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    texture.width = 1;
    texture.height = 1;
    return texture;
}

unsigned int GetFallbackTextureID() {
    const std::string fallbackName = "__fallback__";
    auto it = ResourceManager::Textures.find(fallbackName);
    if (it != ResourceManager::Textures.end()) {
        return it->second.ID;
    }

    Texture2D fallback = CreateFallbackTexture();
    ResourceManager::Textures[fallbackName] = fallback;
    return fallback.ID;
}
} // namespace

unsigned int ResourceManager::LoadTexture(const char* file, const std::string& name) {
    Textures[name] = loadTextureFromFile(file);
    return Textures[name].ID;
}

unsigned int ResourceManager::GetTexture(const std::string& name) {
    auto it = Textures.find(name);
    if (it == Textures.end()) {
        std::cerr << "ERROR::ResourceManager::GetTexture: Texture not found: " << name << std::endl;
        return 0;
    }
    return it->second.ID;
}

void ResourceManager::DeleteTexture(const std::string& name) {
    auto it = Textures.find(name);
    if (it != Textures.end()) {
        glDeleteTextures(1, &it->second.ID);
        Textures.erase(it);
    }
}

unsigned int ResourceManager::LoadShader(const char *vShaderFile, const char *fShaderFile, const std::string &name) {
    Shaders[name] = loadShaderFromFile(vShaderFile, fShaderFile);
    return Shaders[name].ID;
}

unsigned int ResourceManager::GetShader(const std::string& name) {
    return Shaders[name].ID;
}

int ResourceManager::GetUniformLocation(const std::string& shaderName, const std::string& uniformName) {
    auto it = Shaders.find(shaderName);
    if (it == Shaders.end()) {
        std::cerr << "ERROR::ResourceManager::GetUniformLocation: Shader not found: " << shaderName << std::endl;
        return -1;
    }

    ShaderProgram& shader = it->second;
    
    // Check cache
    auto cacheIt = shader.uniformCache.find(uniformName);
    if (cacheIt != shader.uniformCache.end()) {
        return cacheIt->second;
    }

    // Not in cache, query OpenGL
    int location = glGetUniformLocation(shader.ID, uniformName.c_str());
    if (location == -1) {
        // Optional: warn if uniform not found (could be optimized out by compiler)
        // std::cout << "Warning: Uniform '" << uniformName << "' not found in shader '" << shaderName << "'" << std::endl;
    }

    // Store in cache
    shader.uniformCache[uniformName] = location;
    return location;
}

Model& ResourceManager::LoadModel(const char* file, std::string name) {
    Models[name] = loadModelFromFile(file);
    return Models[name];
}

Model& ResourceManager::GetModel(std::string name) {
    return Models[name];
}

void ResourceManager::Clear() {
    for (auto iter : Shaders) glDeleteProgram(iter.second.ID);
    for (auto iter : Textures) glDeleteTextures(1, &iter.second.ID);
    // Models cleanup if needed (VAO/VBO)
}

Texture2D ResourceManager::loadTextureFromFile(const char* file) {
    Texture2D texture{};
    int width, height, nrChannels;
    
    // Flip textures on load
    stbi_set_flip_vertically_on_load(true);
    
    std::string resolvedPath = ResolvePath(file);
    unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &texture.ID);
        glBindTexture(GL_TEXTURE_2D, texture.ID);
        
        // Set alignment to 1 for odd-width textures
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        texture.width = width;
        texture.height = height;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << resolvedPath
                  << " (" << stbi_failure_reason() << ")" << std::endl;
        texture.ID = GetFallbackTextureID();
        texture.width = 1;
        texture.height = 1;
    }

    return texture;
}

ShaderProgram ResourceManager::loadShaderFromFile(const char* vShaderFile, const char* fShaderFile) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFileStream;
    std::ifstream fShaderFileStream;

    vShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        std::string resolvedVertex = ResolvePath(vShaderFile);
        std::string resolvedFragment = ResolvePath(fShaderFile);
        vShaderFileStream.open(resolvedVertex);
        fShaderFileStream.open(resolvedFragment);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFileStream.rdbuf();
        fShaderStream << fShaderFileStream.rdbuf();

        vShaderFileStream.close();
        fShaderFileStream.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    ShaderProgram shader;
    shader.ID = glCreateProgram();
    glAttachShader(shader.ID, vertex);
    glAttachShader(shader.ID, fragment);
    glLinkProgram(shader.ID);
    glGetProgramiv(shader.ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader.ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shader;
}

Model ResourceManager::loadModelFromFile(const char* file) {
    Model model;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string resolvedPath = ResolvePath(file);
    std::filesystem::path modelPath(resolvedPath);
    std::filesystem::path baseDirPath = modelPath.parent_path();

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, resolvedPath.c_str(), baseDirPath.string().c_str())) {
        std::cout << "TinyObjLoader Error: " << warn << err << std::endl;
        return model;
    }

    // For each shape (or group by material to reduce draw calls)
    // Here we just iterate shapes. A better way is to group by material index.
    
    // We will group by material index to create submeshes.
    std::map<int, std::vector<float>> verticesByMaterial;
    std::map<int, unsigned int> materialTextures;

    auto getMaterialTexture = [&](int materialId) -> unsigned int {
        auto texIt = materialTextures.find(materialId);
        if (texIt != materialTextures.end()) {
            return texIt->second;
        }

        unsigned int textureId = 0;
        if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {
            const auto& mat = materials[materialId];
            if (!mat.diffuse_texname.empty()) {
                std::filesystem::path texPath = baseDirPath / mat.diffuse_texname;
                std::string resolvedTexPath = ResolvePath(texPath.string().c_str());
                auto cached = Textures.find(resolvedTexPath);
                if (cached != Textures.end()) {
                    textureId = cached->second.ID;
                } else {
                    Textures[resolvedTexPath] = loadTextureFromFile(resolvedTexPath.c_str());
                    textureId = Textures[resolvedTexPath].ID;
                }
            }
        }

        if (textureId == 0) {
            textureId = GetFallbackTextureID();
        }

        materialTextures[materialId] = textureId;
        return textureId;
    };

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        // For each face
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            int materialId = -1;
            if (f < shape.mesh.material_ids.size()) {
                materialId = shape.mesh.material_ids[f];
            }
            std::vector<float>& vertices = verticesByMaterial[materialId];
            
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                
                // Position
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
                
                // TexCoord
                if (idx.texcoord_index >= 0) {
                    vertices.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                    vertices.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
                
                // Normal
                if (idx.normal_index >= 0) {
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 2]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }
            }
            index_offset += fv;
        }
    }

    for (const auto& [materialId, vertices] : verticesByMaterial) {
        if (vertices.empty()) {
            continue;
        }

        SubMesh sm;
        glGenVertexArrays(1, &sm.vao);
        glGenBuffers(1, &sm.vbo);

        glBindVertexArray(sm.vao);
        glBindBuffer(GL_ARRAY_BUFFER, sm.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Tex
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Normal
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        sm.vertexCount = vertices.size() / 8;
        sm.textureID = getMaterialTexture(materialId);

        model.push_back(sm);
    }
    
    return model;
}

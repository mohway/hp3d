
#include "resource_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

std::map<std::string, Texture2D>     ResourceManager::Textures;
std::map<std::string, ShaderProgram> ResourceManager::Shaders;
std::map<std::string, Model>         ResourceManager::Models;

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

int ResourceManager::GetUniformLocation(const std::string& shaderName, const std::string& uniformName) {
    if (!Shaders.contains(shaderName)) {
        std::cout << "Warning: Shader " << shaderName << " not found when getting uniform " << uniformName << std::endl;
        return -1;
    }

    ShaderProgram& shader = Shaders[shaderName];
    if (shader.uniformCache.contains(uniformName)) {
        return shader.uniformCache[uniformName];
    }

    int location = glGetUniformLocation(shader.ID, uniformName.c_str());
    shader.uniformCache[uniformName] = location;
    return location;
}

void ResourceManager::Clear() {
    for (const auto& iter : Textures) glDeleteTextures(1, &iter.second.ID);
    for (const auto& iter : Shaders) glDeleteProgram(iter.second.ID);
    for (auto& iter : Models) {
        for (auto& mesh : iter.second) {
            glDeleteVertexArrays(1, &mesh.vao);
            glDeleteBuffers(1, &mesh.vbo);
        }
    }
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
        
        // Fix alignment for odd-width textures
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
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

Model& ResourceManager::LoadModel(const char* file, std::string name) {
    if (Models.find(name) != Models.end())
        return Models[name];

    Models[name] = loadModelFromFile(file);
    return Models[name];
}

Model& ResourceManager::GetModel(std::string name) {
    return Models[name];
}

Model ResourceManager::loadModelFromFile(const char* objPath) {
    Model model;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Handle base directory for textures
    std::string baseDir = objPath;
    if (baseDir.find_last_of("/\\") != std::string::npos) {
        baseDir = baseDir.substr(0, baseDir.find_last_of("/\\") + 1);
    } else {
        baseDir = "";
    }

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath, baseDir.c_str());

    if (!warn.empty()) std::cout << "OBJ Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "OBJ Error: " << err << std::endl;
    if (!ret) return model;

    // Group Geometry by Material
    std::map<int, std::vector<float>> sortedGeometry;

    for (const auto&[name, mesh, lines, points] : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
            int currentMaterialId = mesh.material_ids[f];
            if (currentMaterialId < 0) currentMaterialId = 0;

            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = mesh.indices[index_offset + v];

                // Pos
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                sortedGeometry[currentMaterialId].push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                // Tex
                if (idx.texcoord_index >= 0) {
                    sortedGeometry[currentMaterialId].push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                    sortedGeometry[currentMaterialId].push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                } else {
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                }

                // Normal
                if (idx.normal_index >= 0) {
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 0]);
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 1]);
                    sortedGeometry[currentMaterialId].push_back(attrib.normals[3 * idx.normal_index + 2]);
                } else {
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                    sortedGeometry[currentMaterialId].push_back(1.0f);
                    sortedGeometry[currentMaterialId].push_back(0.0f);
                }
            }
            index_offset += 3;
        }
    }

    // Create SubMeshes
    for (auto& [matID, data] : sortedGeometry) {
        SubMesh subMesh = {};

        // Texture Loading
        if (matID < materials.size() && !materials[matID].diffuse_texname.empty()) {
            std::string texName = materials[matID].diffuse_texname;
            std::string texPath = baseDir + texName;
            subMesh.textureID = ResourceManager::LoadTexture(texPath.c_str(), texName);
        } else {
            subMesh.textureID = ResourceManager::GetTexture("zwin_floor"); // Default fallback
        }

        subMesh.vertexCount = data.size() / 8; // 8 floats per vertex

        glGenVertexArrays(1, &subMesh.vao);
        glGenBuffers(1, &subMesh.vbo);
        glBindVertexArray(subMesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, subMesh.vbo);

        // Upload directly from the vector.
        // This is safe because 'data' exists until the end of this loop scope.
        // We don't need the Arena here unless we want to keep CPU data persistently.
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

        glBindVertexArray(0);
        model.push_back(subMesh);
    }

    std::cout << "Loaded Model: " << objPath << " (" << model.size() << " submeshes)" << std::endl;
    return model;
}

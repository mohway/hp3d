#pragma once

#include <map>
#include <string>
#include <glad/glad.h>

struct Texture2D {
    unsigned int ID;
    int width, height;
};

struct ShaderProgram {
    unsigned int ID;
    void use() { glUseProgram(ID); }
};

class ResourceManager {
public:
    static std::map<std::string, Texture2D>     Textures;
    static std::map<std::string, ShaderProgram> Shaders;

    static unsigned int LoadTexture(const char* file, const std::string& name);
    static unsigned int GetTexture(const std::string& name);

    static unsigned int LoadShader(const char *vShaderFile, const char *fShaderFile, const std::string &name);
    static unsigned int GetShader(const std::string& name);

    static void Clear();

private:
    ResourceManager() {}

    static Texture2D loadTextureFromFile(const char* file);
    static ShaderProgram loadShaderFromFile(const char* vShaderFile, const char* fShaderFile);
};

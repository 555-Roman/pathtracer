#ifndef PATHTRACER_INCLUDE_H
#define PATHTRACER_INCLUDE_H

#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "stb_image/stb_image.h"

using namespace glm;

struct Material {
    vec3 colour;
    float padding0;
    vec3 emissionColour;
    float emissionStrength;
    GLuint64 textureHandle;
    GLuint64 padding1;
};

inline GLuint64 getTexture(const char* filePath) {
    unsigned int cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePath, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint64 cubeTextureHandle = glGetTextureHandleARB(cubeTexture);
    glMakeTextureHandleResidentARB(cubeTextureHandle);

    return cubeTextureHandle;
}

struct Triangle {
    vec3 a;
    float padding_a;
    vec3 b;
    float padding_b;
    vec3 c;
    float padding_c;
    vec2 uv_a;
    vec2 uv_b;
    vec2 uv_c;
    vec2 uv_padding;
};

struct Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];
    vec3 offset;
    float scale;
    Material material;
};

inline std::vector<Triangle> triangles;
inline std::vector<Model> models;

#endif //PATHTRACER_INCLUDE_H
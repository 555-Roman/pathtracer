#ifndef PATHTRACER_INCLUDE_H
#define PATHTRACER_INCLUDE_H

#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <chrono>

#include "glm/ext/matrix_transform.hpp"
#include "stb_image/stb_image.h"

using namespace glm;

struct Material {
    vec3 albedo;
    float opacity;
    vec3 emissionColour;
    float emissionStrength;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    vec3 complexN;
    float padding0;
    vec3 complexK;
    float padding1;
    GLuint64 albedoTextureHandle;
    GLuint64 roughnessTextureHandle;
    GLuint64 metalnessTextureHandle;
    GLuint64 normalTextureHandle;
};

inline constexpr Material defaultMaterial = {
    vec3(0.8),
    1.0,
    vec3(0.0),
    0.0,
    1.0,
    0.0,
    1.5,
    0.0,
    vec3(0.0),
    0.0,
    vec3(0.0),
    0.0,
    0,
    0,
    0,
    0
};

struct Triangle {
    vec3 aPosition;
    float aU;
    vec3 bPosition;
    float bU;
    vec3 cPosition;
    float cU;
    vec3 aNormal;
    float aV;
    vec3 bNormal;
    float bV;
    vec3 cNormal;
    float cV;
};

struct BVHNode {
    vec3 minBound;
    uint index;
    vec3 maxBound;
    uint triangleCount;
};

struct GPU_Model {
    uint bvhNodeIndex;
    uint padding[3];

    Material material;

    mat4 modelWorldMatrix;
    mat4 worldModelMatrix;
};

struct Model {
    uint bvhNodeIndex = 0;

    vec3 translation = vec3(0.0);
    vec3 rotation = vec3(0.0);
    vec3 scale = vec3(1.0);

    mat4 modelWorldMatrix;
    mat4 worldModelMatrix;

    Material material = defaultMaterial;

    void updateMatrices() {
        worldModelMatrix = mat4(1.0);

        worldModelMatrix = translate(worldModelMatrix, translation);

        worldModelMatrix = rotate(worldModelMatrix, rotation.y, vec3(0.0, -1.0, 0.0));
        worldModelMatrix = rotate(worldModelMatrix, rotation.x, vec3(-1.0, 0.0, 0.0));
        worldModelMatrix = rotate(worldModelMatrix, rotation.z, vec3(0.0, 0.0, -1.0));

        worldModelMatrix = glm::scale(worldModelMatrix, scale);

        modelWorldMatrix = inverse(worldModelMatrix);
    }

    GPU_Model gpuModel() {
        GPU_Model result{};

        result.bvhNodeIndex = bvhNodeIndex;

        result.material = material;

        updateMatrices();
        result.modelWorldMatrix = modelWorldMatrix;
        result.worldModelMatrix = worldModelMatrix;

        return result;
    }
};

inline std::vector<Triangle> triangles;
inline GLuint triangle_ssbo;
inline std::vector<BVHNode> bvhNodes;
inline GLuint bvhNode_ssbo;
inline std::vector<Model> models;
inline std::vector<GPU_Model> gpuModels;
inline GLuint model_ssbo;

inline GLuint64 getTexture(const char* filePath) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
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
        std::cout << "Failed to load texture: " << filePath << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint64 textureHandle = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(textureHandle);

    return textureHandle;
}

#endif //PATHTRACER_INCLUDE_H
#include "skybox.h"

void setSkyboxCubemap(const char* folderPath, const char* extension) {
    stbi_set_flip_vertically_on_load(false);

    skyboxFormat = 1;

    if (skyboxCubemapTextureID == 0) glGenTextures(1, &skyboxCubemapTextureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapTextureID);

    const std::vector<std::string> faces = {"right", "left", "top", "bottom", "front", "back"};
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        std::string fullPath = std::string(folderPath);
        fullPath += "/";
        fullPath += faces[i];
        fullPath += std::string(extension);
        unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture failed to load at path: " << fullPath << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    skyboxCubemapTexture = glGetTextureHandleARB(skyboxCubemapTextureID);
    glMakeTextureHandleResidentARB(skyboxCubemapTexture);

    stbi_set_flip_vertically_on_load(true);
}

void setSkyboxCubemapTexture(const char *filePath, GLenum target) {
    stbi_set_flip_vertically_on_load(false);

    skyboxFormat = 1;

    if (skyboxCubemapTextureID == 0) glGenTextures(1, &skyboxCubemapTextureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapTextureID);

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

        glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        std::cout << "Cubemap texture failed to load at path: " << filePath << std::endl;
        stbi_image_free(data);
    }

    skyboxCubemapTexture = glGetTextureHandleARB(skyboxCubemapTextureID);
    glMakeTextureHandleResidentARB(skyboxCubemapTexture);

    stbi_set_flip_vertically_on_load(true);
}

void setSkyboxCubemapRight(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
}
void setSkyboxCubemapLeft(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
}
void setSkyboxCubemapTop(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
}
void setSkyboxCubemapBottom(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
}
void setSkyboxCubemapFront(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
}
void setSkyboxCubemapBack(const char* filePath) {
    setSkyboxCubemapTexture(filePath, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
}

void setSkyboxEquirectangular(const char* filePath) {
    skyboxFormat = 2;

    if (skyboxEquirectangularTextureID == 0) glGenTextures(1, &skyboxEquirectangularTextureID);
    glBindTexture(GL_TEXTURE_2D, skyboxEquirectangularTextureID);

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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Equirectangular texture failed to load at path: " << filePath << std::endl;
        stbi_image_free(data);
    }

    skyboxEquirectangularTexture = glGetTextureHandleARB(skyboxEquirectangularTextureID);
    glMakeTextureHandleResidentARB(skyboxEquirectangularTexture);
}

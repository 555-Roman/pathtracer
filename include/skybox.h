#ifndef PATHTRACER_SKYBOX_H
#define PATHTRACER_SKYBOX_H

#include "include.h"

void setSkyboxCubemap(const char* folderPath, const char* extension);
void setSkyboxCubemapTexture(const char* filePath, GLenum target);
void setSkyboxCubemapRight(const char* filePath);
void setSkyboxCubemapLeft(const char* filePath);
void setSkyboxCubemapTop(const char* filePath);
void setSkyboxCubemapBottom(const char* filePath);
void setSkyboxCubemapBack(const char* filePath);
void setSkyboxCubemapFront(const char* filePath);

void setSkyboxEquirectangular(const char* filePath);

inline uint skyboxFormat = 0;
inline uint skyboxCubemapTextureID = 0;
inline GLuint64 skyboxCubemapTexture;
inline uint skyboxEquirectangularTextureID = 0;
inline GLuint64 skyboxEquirectangularTexture;

#endif //PATHTRACER_SKYBOX_H
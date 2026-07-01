#version 460 core

layout (location = 0) in vec2 pos;

uniform uvec2 halfScreenSize;
uniform mat3 cameraRotation;

out vec3 originalRayDir;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    originalRayDir = cameraRotation * vec3(pos*halfScreenSize, -1.0 * halfScreenSize.x);
}

#version 460 core

layout (location = 0) in vec2 pos;

uniform uvec2 halfScreenSize;
uniform mat3 cameraRotation;
uniform float fov;

out vec3 originalRayDir;
out vec2 uv;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);

    float radiansFov = radians(fov);
    float focalLength = halfScreenSize.x / tan(radiansFov * 0.5);

    originalRayDir = cameraRotation * vec3(pos*halfScreenSize, -focalLength);
    uv = pos * 0.5 + 0.5;
}

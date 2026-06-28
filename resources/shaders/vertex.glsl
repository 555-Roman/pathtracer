#version 460 core

layout (location = 0) in vec2 pos;

out vec3 originalRayDir;

void main() {
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
    originalRayDir = vec3(pos.x*320, pos.y*240, -320);
}

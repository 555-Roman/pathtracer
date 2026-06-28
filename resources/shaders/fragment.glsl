#version 330 core

in vec3 originalRayDir;

out vec4 FragColor;

void main() {
    FragColor = vec4(normalize(originalRayDir), 1.0);
}

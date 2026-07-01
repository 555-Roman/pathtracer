#version 460

in vec2 uv;

uniform sampler2D accumulatedTexture;

out vec4 FragColor;

void main() {
    FragColor = vec4(pow(texture(accumulatedTexture, uv).rgb, vec3(1.0/2.2)), 1.0);
}
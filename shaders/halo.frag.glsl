#version 330 core

in vec2 vCorner;
out vec4 fragColor;

uniform vec3 uColor; // intensity folded in

void main() {
    float r = length(vCorner);
    float g = pow(max(1.0 - r, 0.0), 2.5);
    fragColor = vec4(uColor * g, 1.0);
}

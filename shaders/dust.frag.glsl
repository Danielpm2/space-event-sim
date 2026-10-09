#version 330 core

in float vAlpha;
out vec4 fragColor;

uniform vec3 uColor;

void main() {
    fragColor = vec4(uColor * vAlpha, 1.0);
}

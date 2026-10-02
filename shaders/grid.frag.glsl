#version 330 core

#include "common/hdr.glsl"

in float vAlpha;
in float vY;
out vec4 fragColor;

uniform float uBaseY;
uniform float uIntensity;

void main() {
    float depth = clamp((uBaseY - vY) / 3.0, 0.0, 1.0);
    vec3 col = mix(vec3(0.12, 0.35, 0.85), vec3(1.0, 0.65, 0.3), depth);
    fragColor = vec4(toLinearClamped(col * vAlpha * uIntensity), 1.0);
}

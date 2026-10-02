#version 330 core

#include "common/hdr.glsl"

in float vW;
out vec4 fragColor;

uniform float uTime;
uniform float uFlare;
uniform float uBrightness;

void main() {
    float id = floor(vW);
    float u = fract(vW);

    // Energy pulses travelling along each line.
    float pulse = pow(0.5 + 0.5 * sin(u * 18.0 - uTime * 3.0 + id * 2.1), 8.0);
    float ends = smoothstep(0.0, 0.05, u) * smoothstep(1.0, 0.95, u);

    vec3 base = mix(vec3(0.15, 0.55, 1.0), vec3(0.75, 0.3, 1.0), fract(id * 0.173));
    vec3 col = base * (0.25 + 1.5 * pulse) * ends;
    col = mix(col, vec3(1.0, 0.85, 0.6) * (0.4 + pulse) * ends, clamp(uFlare, 0.0, 1.0));
    fragColor = vec4(toLinearHdr(col * uBrightness, 3.0), 1.0);
}

#version 330 core

#include "common/hdr.glsl"

in float vLife;
out vec4 fragColor;

uniform float uGlow;
uniform vec3 uTintNew; // colour at birth
uniform vec3 uTintOld; // colour near end of life

void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float r = dot(p, p);
    if (r > 1.0)
        discard;
    float g = (1.0 - r) * (1.0 - r);
    vec3 col = mix(uTintOld, uTintNew, vLife);
    fragColor = vec4(toLinearClamped(col * g * vLife * 0.9 * uGlow), 1.0);
}

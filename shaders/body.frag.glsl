#version 330 core

#include "common/hdr.glsl"

in vec3 vNormal;
in vec3 vWorld;
out vec4 fragColor;

uniform vec3 uCamPos;
uniform vec3 uColor;
uniform float uBrightness;

void main() {
    vec3 n = normalize(vNormal);
    vec3 v = normalize(uCamPos - vWorld);
    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    float core = 0.55 + 0.45 * max(dot(n, v), 0.0);
    vec3 col = uColor * core + rim * vec3(0.6, 0.85, 1.0);
    fragColor = vec4(toLinearHdr(col * uBrightness, 2.5), 1.0);
}

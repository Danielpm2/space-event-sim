#version 330 core

#include "common/hdr.glsl"

// Translucent expanding shell, bright toward its silhouette. Pairs with body.vert.glsl.
in vec3 vNormal;
in vec3 vWorld;
out vec4 fragColor;

uniform vec3 uCamPos;
uniform vec3 uColor;
uniform float uStrength;

void main() {
    float f = 1.0 - abs(dot(normalize(vNormal), normalize(uCamPos - vWorld)));
    fragColor = vec4(toLinearHdr(uColor * pow(f, 2.0) * uStrength, 3.0), 1.0);
}

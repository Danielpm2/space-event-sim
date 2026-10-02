#version 330 core

#include "common/hdr.glsl"

in vec3 vNormal;
in vec3 vWorld;
out vec4 fragColor;

uniform vec3 uCamPos;
uniform vec3 uMagAxis;
uniform float uPhase;
uniform float uGlow;
uniform vec3 uHotspot;          // optional glowing surface spot (unset = off)
uniform float uHotspotStrength;

void main() {
    vec3 n = normalize(vNormal);
    vec3 v = normalize(uCamPos - vWorld);

    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    float pole = pow(abs(dot(n, uMagAxis)), 28.0);

    // Faint surface pattern that rotates with the star.
    float lon = atan(n.z, n.x) - uPhase;
    float bands = 0.9 + 0.1 * sin(lon * 5.0) * sin(n.y * 9.0);

    vec3 col = vec3(0.45, 0.65, 1.0) * bands * 0.85;
    col += rim * vec3(0.5, 0.8, 1.0) * 1.3;
    col += pole * vec3(1.0) * 1.6;
    col += pow(max(dot(n, uHotspot), 0.0), 18.0) * uHotspotStrength * vec3(1.0, 0.6, 0.25) * 3.0;
    fragColor = vec4(toLinearClamped(col * (0.6 + 0.4 * uGlow)), 1.0);
}

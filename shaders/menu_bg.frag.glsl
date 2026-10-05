#version 330 core

#include "common/hdr.glsl"
#include "common/stars.glsl"

in vec2 vUV;
out vec4 fragColor;

uniform vec2 uResolution;
uniform float uTime;

const float TAN_HALF_FOV = 0.58;

// Same tone mapping as composite.frag.glsl; the menu skips the HDR pipeline.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec2 ndc = vUV * 2.0 - 1.0;
    vec3 ray = normalize(vec3(ndc.x * uResolution.x / uResolution.y * TAN_HALF_FOV, ndc.y * TAN_HALF_FOV, -1.0));

    // Slow drift across the sky.
    float yaw = uTime * 0.025, pitch = 0.35 + 0.08 * sin(uTime * 0.05);
    float cy = cos(yaw), sy = sin(yaw), cp = cos(pitch), sp = sin(pitch);
    ray = vec3(ray.x, ray.y * cp - ray.z * sp, ray.y * sp + ray.z * cp);
    ray = vec3(ray.x * cy + ray.z * sy, ray.y, -ray.x * sy + ray.z * cy);

    vec3 c = aces(toLinearClamped(starfield(ray)) * 0.9);
    c *= 1.0 - 0.45 * dot(ndc, ndc) * 0.5; // vignette
    fragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);
}

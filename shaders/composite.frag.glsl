#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uBloomIntensity; // 0 when bloom is off
uniform float uExposure;

// Narkowicz ACES filmic approximation.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 c = max(texture(uScene, vUV).rgb, vec3(0.0));
    c += max(texture(uBloom, vUV).rgb, vec3(0.0)) * uBloomIntensity;
    c = aces(c * uExposure);
    fragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);
}

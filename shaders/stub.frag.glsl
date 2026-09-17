#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform vec2 uResolution;
uniform float uTime;
uniform float uHue;

vec3 hsv(float h, float s, float v) {
    vec3 k = clamp(abs(fract(h + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
    return v * mix(vec3(1.0), k, s);
}

void main() {
    vec2 p = (vUV * 2.0 - 1.0) * vec2(uResolution.x / uResolution.y, 1.0);
    float r = length(p);
    float ring = 0.5 + 0.5 * sin(r * 12.0 - uTime * 3.0);
    fragColor = vec4(hsv(uHue, 0.8, ring * exp(-r)), 1.0);
}

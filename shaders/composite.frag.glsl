#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform float uExposure;

// Narkowicz ACES filmic approximation.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 c = max(texture(uScene, vUV).rgb, vec3(0.0)) * uExposure;
    c = aces(c);
    fragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);
}

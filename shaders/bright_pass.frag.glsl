#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform float uThreshold;

vec3 brightPart(vec2 uv) {
    vec3 c = texture(uScene, uv).rgb;
    if (any(isnan(c)) || any(isinf(c)))
        return vec3(0.0);
    c = clamp(c, vec3(0.0), vec3(64.0)); // tames single-pixel fireflies
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return c * (max(lum - uThreshold, 0.0) / max(lum, 1e-4));
}

void main() {
    // 4 taps over the full-res source give a clean 2x downsample.
    vec2 t = 0.5 / vec2(textureSize(uScene, 0));
    vec3 sum = brightPart(vUV + vec2(-t.x, -t.y)) + brightPart(vUV + vec2(t.x, -t.y)) +
               brightPart(vUV + vec2(-t.x, t.y)) + brightPart(vUV + vec2(t.x, t.y));
    fragColor = vec4(sum * 0.25, 1.0);
}

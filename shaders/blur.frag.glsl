#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uImage;
uniform vec2 uDirection; // one texel step along the blur axis, in UV units

// 9-tap Gaussian (sigma ~ 2 texels).
const float W[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec3 sum = texture(uImage, vUV).rgb * W[0];
    for (int i = 1; i < 5; ++i) {
        vec2 o = uDirection * float(i);
        sum += texture(uImage, vUV + o).rgb * W[i];
        sum += texture(uImage, vUV - o).rgb * W[i];
    }
    fragColor = vec4(sum, 1.0);
}

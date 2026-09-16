#version 330 core

in vec2 vUV;
out vec4 fragColor;

uniform vec2 uResolution;
uniform float uTime;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec2 px = vUV * uResolution;
    vec3 col = mix(vec3(0.01, 0.01, 0.03), vec3(0.03, 0.02, 0.07), vUV.y);

    // Two grid layers of slowly twinkling stars.
    for (int i = 0; i < 2; ++i) {
        float cell = i == 0 ? 60.0 : 110.0;
        vec2 g = px / cell + float(i) * 17.0;
        vec2 id = floor(g);
        vec2 f = fract(g) - 0.5;
        float h = hash(id);
        if (h > 0.82) {
            vec2 off = vec2(hash(id + 3.1), hash(id + 7.7)) - 0.5;
            float d = length(f - off * 0.6);
            float tw = 0.6 + 0.4 * sin(uTime * (1.0 + h * 3.0) + h * 40.0);
            col += vec3(0.8, 0.85, 1.0) * smoothstep(0.08, 0.0, d) * tw;
        }
    }
    fragColor = vec4(col, 1.0);
}

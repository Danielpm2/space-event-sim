#version 330 core

in float vLife;
out vec4 fragColor;

uniform float uGlow;

void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float r = dot(p, p);
    if (r > 1.0)
        discard;
    float g = (1.0 - r) * (1.0 - r);
    vec3 col = mix(vec3(0.25, 0.45, 1.0), vec3(0.8, 0.95, 1.0), vLife);
    fragColor = vec4(col * g * vLife * 0.9 * uGlow, 1.0);
}

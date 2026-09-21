#version 330 core

layout(location = 0) in vec4 aPosLife; // xyz position, w = remaining life 0..1

uniform mat4 uViewProj;
uniform float uPointSize;
uniform float uScreenHeight;

out float vLife;

void main() {
    gl_Position = uViewProj * vec4(aPosLife.xyz, 1.0);
    vLife = aPosLife.w;
    gl_PointSize = clamp(uPointSize * uScreenHeight / max(gl_Position.w, 0.1), 1.0, 24.0);
}

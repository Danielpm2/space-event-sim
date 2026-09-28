#version 330 core

layout(location = 0) in vec4 aPosAlpha; // xyz position, w = opacity

uniform mat4 uViewProj;

out float vAlpha;
out float vY;

void main() {
    vAlpha = aPosAlpha.w;
    vY = aPosAlpha.y;
    gl_Position = uViewProj * vec4(aPosAlpha.xyz, 1.0);
}

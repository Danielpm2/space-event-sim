#version 330 core

layout(location = 0) in vec4 aPosAlpha; // xyz position, w = brightness at this end of the streak

uniform mat4 uViewProj;

out float vAlpha;

void main() {
    gl_Position = uViewProj * vec4(aPosAlpha.xyz, 1.0);
    vAlpha = aPosAlpha.w;
}

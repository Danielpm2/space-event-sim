#version 330 core

layout(location = 0) in vec4 aPosW; // xyz position, w = lineId + parameter along the line (0..1)

uniform mat4 uViewProj;

out float vW;

void main() {
    vW = aPosW.w;
    gl_Position = uViewProj * vec4(aPosW.xyz, 1.0);
}

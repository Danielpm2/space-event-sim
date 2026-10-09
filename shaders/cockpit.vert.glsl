#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in float aMat;

uniform mat4 uProj;
uniform mat4 uModel;
uniform float uXScale; // keeps the frame at the screen edges on any aspect ratio

out vec3 vPos;
out vec3 vNormal;
out vec2 vUV;
flat out float vMat;

void main() {
    vec3 p = aPos;
    p.x *= uXScale;
    vec4 v = uModel * vec4(p, 1.0);
    vPos = v.xyz;
    vNormal = mat3(uModel) * aNormal;
    vUV = aUV;
    vMat = aMat;
    gl_Position = uProj * v;
}

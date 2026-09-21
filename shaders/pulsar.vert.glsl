#version 330 core

layout(location = 0) in vec3 aPos; // unit sphere

uniform mat4 uViewProj;
uniform float uRadius;

out vec3 vNormal;
out vec3 vWorld;

void main() {
    vNormal = aPos;
    vWorld = aPos * uRadius;
    gl_Position = uViewProj * vec4(vWorld, 1.0);
}

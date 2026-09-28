#version 330 core

layout(location = 0) in vec3 aPos; // unit sphere

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform mat3 uNormalMat;

out vec3 vNormal;
out vec3 vWorld;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vNormal = uNormalMat * aPos;
    gl_Position = uViewProj * world;
}

#version 330 core

// Cone surface parameterised by (angle around the axis, t along the axis).
layout(location = 0) in vec2 aParam;

uniform mat4 uViewProj;
uniform vec3 uAxis;
uniform vec3 uU;
uniform vec3 uV;
uniform float uLength;
uniform float uTanHalf;

out float vT;
out vec3 vNormal;
out vec3 vWorld;

void main() {
    vec3 radial = cos(aParam.x) * uU + sin(aParam.x) * uV;
    float d = aParam.y * uLength;
    vWorld = uAxis * d + radial * (d * uTanHalf);

    float cosA = inversesqrt(1.0 + uTanHalf * uTanHalf);
    vNormal = radial * cosA - uAxis * (uTanHalf * cosA);
    vT = aParam.y;
    gl_Position = uViewProj * vec4(vWorld, 1.0);
}

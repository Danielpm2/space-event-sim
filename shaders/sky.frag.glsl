#version 330 core

#include "common/camera.glsl"
#include "common/hdr.glsl"
#include "common/stars.glsl"

in vec2 vUV;
out vec4 fragColor;

void main() {
    fragColor = vec4(toLinearClamped(starfield(cameraRay(vUV))), 1.0);
}

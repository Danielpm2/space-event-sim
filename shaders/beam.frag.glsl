#version 330 core

in float vT;
in vec3 vNormal;
in vec3 vWorld;
out vec4 fragColor;

uniform vec3 uCamPos;
uniform vec3 uColor;
uniform float uIntensity;
uniform float uFlash;

void main() {
    // Surfaces seen edge-on are thin; face-on ones look like dense glowing volume.
    float facing = abs(dot(normalize(vNormal), normalize(uCamPos - vWorld)));
    float body = max(pow(facing, 1.5), uFlash);
    float fade = pow(1.0 - vT, 1.6) * smoothstep(0.0, 0.05, vT);
    float a = body * fade * uIntensity;
    fragColor = vec4(uColor * a, 1.0);
}

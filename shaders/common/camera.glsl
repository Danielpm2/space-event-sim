// Camera description shared by full-screen ray shaders (set via setCameraUniforms).
uniform vec3 uCamPos;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform vec3 uCamForward;
uniform float uTanHalfFov;
uniform vec2 uResolution;

// uv in [0,1]^2 over the screen -> world-space ray direction.
vec3 cameraRay(vec2 uv) {
    vec2 ndc = uv * 2.0 - 1.0;
    float aspect = uResolution.x / uResolution.y;
    return normalize(uCamForward + uCamRight * (ndc.x * aspect * uTanHalfFov) + uCamUp * (ndc.y * uTanHalfFov));
}

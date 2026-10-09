#version 330 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vColor;
out vec4 fragColor;

uniform sampler2D uBase;
uniform sampler2D uMetalRough; // glTF packing: R occlusion, G roughness, B metallic
uniform sampler2D uNormalMap;
uniform sampler2D uEmissive;
uniform int uHasBase, uHasMetalRough, uHasNormal, uHasEmissive;
uniform vec4 uBaseFactor;
uniform float uMetallic, uRoughness;
uniform vec3 uEmissiveFactor;
uniform int uAlphaMode; // 0 opaque, 1 mask, 2 blend
uniform float uAlphaCutoff;

uniform vec3 uEventDir, uEventColor;
uniform vec3 uCabinDir, uCabinColor;
uniform vec3 uAmbient;
uniform float uEmissiveScale;
uniform float uTime;

const float PI = 3.14159265;

vec3 shade(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metal, float rough) {
    float NL = max(dot(N, L), 0.0);
    if (NL <= 0.0)
        return vec3(0.0);
    vec3 H = normalize(V + L);
    float NH = max(dot(N, H), 0.0);
    float NV = max(dot(N, V), 1e-3);
    float VH = max(dot(V, H), 0.0);
    float a = max(rough * rough, 0.02);
    float a2 = a * a;
    float d = NH * NH * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * d * d);
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VH, 5.0);
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    float G = (NL / (NL * (1.0 - k) + k)) * (NV / (NV * (1.0 - k) + k));
    vec3 spec = D * F * G / (4.0 * NL * NV + 1e-3);
    vec3 diff = (1.0 - F) * (1.0 - metal) * albedo / PI;
    return (diff + spec) * radiance * NL;
}

void main() {
    vec4 base = uBaseFactor;
    if (uHasBase == 1)
        base *= texture(uBase, vUV);
    base.rgb *= vColor.rgb;
    if (uAlphaMode == 1 && base.a < uAlphaCutoff)
        discard;

    float ao = 1.0, rough = uRoughness, metal = uMetallic;
    if (uHasMetalRough == 1) {
        vec3 orm = texture(uMetalRough, vUV).rgb;
        ao = orm.r;
        rough *= orm.g;
        metal *= orm.b;
    }

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing)
        N = -N;
    if (uHasNormal == 1) {
        vec3 uvDx = dFdx(vec3(vUV, 0.0)), uvDy = dFdy(vec3(vUV, 0.0));
        float det = uvDx.s * uvDy.t - uvDy.s * uvDx.t;
        if (abs(det) > 1e-12) {
            vec3 t = (uvDy.t * dFdx(vPos) - uvDx.t * dFdy(vPos)) / det;
            t = normalize(t - N * dot(N, t));
            vec3 b = cross(N, t);
            vec3 nm = texture(uNormalMap, vUV).rgb * 2.0 - 1.0;
            N = normalize(mat3(t, b, N) * nm);
        }
    }
    vec3 V = normalize(-vPos);

    vec3 col = shade(N, V, normalize(uEventDir), uEventColor, base.rgb, metal, rough);
    col += shade(N, V, normalize(uCabinDir), uCabinColor, base.rgb, metal, rough);
    col += uAmbient * base.rgb * (1.0 - metal) * ao;
    if (uHasEmissive == 1)
        col += uEmissiveFactor * texture(uEmissive, vUV).rgb * uEmissiveScale;
    else
        col += uEmissiveFactor * uEmissiveScale;

    float alpha = uAlphaMode == 2 ? base.a : 1.0;
    fragColor = vec4(col, alpha);
}

#version 330 core

#include "common/camera.glsl"
#include "common/hdr.glsl"
#include "common/stars.glsl"
#include "common/noise.glsl"

in vec2 vUV;
out vec4 fragColor;

uniform float uTime;
uniform float uStarRadius;
uniform float uStarBright;
uniform float uFlash;
uniform float uShockR;
uniform float uThick;
uniform float uShellBright;
uniform float uClump;
uniform float uFade;
uniform float uRemnant;

const int STEPS = 56;

vec3 heat(float t) {
    vec3 c = mix(vec3(0.35, 0.04, 0.1), vec3(1.0, 0.33, 0.06), smoothstep(0.0, 0.4, t));
    c = mix(c, vec3(1.0, 0.78, 0.38), smoothstep(0.4, 0.85, t));
    return mix(c, vec3(0.85, 0.92, 1.0), smoothstep(0.95, 1.5, t));
}

void main() {
    vec3 ro = uCamPos;
    vec3 rd = cameraRay(vUV);

    vec3 col = vec3(0.0);
    float trans = 1.0;

    float outer = max(uShockR + 2.5 * uThick, uStarRadius * 1.3 + 0.1);
    float b = dot(ro, rd);
    float disc = b * b - (dot(ro, ro) - outer * outer);

    if (disc > 0.0) {
        float s = sqrt(disc);
        float t0 = max(-b - s, 0.0);
        float t1 = -b + s;
        float dt = (t1 - t0) / float(STEPS);
        float jitter = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);

        for (int i = 0; i < STEPS; ++i) {
            vec3 p = ro + rd * (t0 + (float(i) + jitter) * dt);
            float r = length(p);
            vec3 dir = p / max(r, 1e-3);

            // Clumpy ejecta: noise varies with direction, slowly with radius.
            float n = fbm3(dir * 3.2 + vec3(r * 0.22) + vec3(0.0, uTime * 0.03, 0.0));
            float n2 = fbm3(dir * 7.0 - vec3(r * 0.4));

            vec3 emit = vec3(0.0);
            float sigma = 0.0;

            if (uShockR > 0.0) {
                float rr = r - (n - 0.5) * uClump * uThick * 2.2;
                float d = (rr - uShockR * 0.9) / uThick;
                float shell = exp(-d * d * 2.0);
                float cavity = (1.0 - smoothstep(0.0, uShockR, r)) * 0.08 * (0.4 + n);
                float fil = mix(1.0, smoothstep(0.32, 0.78, n), uClump);
                float dens = shell * (0.15 + 2.0 * fil) + cavity;

                float temp = uShellBright * (0.35 + 0.9 * shell) * (1.0 - 0.35 * r / (uShockR + uThick));
                vec3 tint = mix(vec3(1.0, 0.35, 0.15), vec3(0.25, 0.85, 0.75), smoothstep(0.45, 0.8, n2));
                vec3 c = mix(heat(temp), tint * temp * 1.4, 0.3 * uClump);
                emit = c * uShellBright * 0.85 * uFade;
                sigma = dens * dt * 0.9;
            }

            if (uStarRadius > 0.0) {
                float sd = exp(-pow(r / uStarRadius, 4.0) * 1.5);
                vec3 sc = mix(vec3(1.0, 0.55, 0.25), vec3(0.85, 0.92, 1.0), smoothstep(1.0, 6.0, uStarBright));
                float ss = sd * 6.0 * dt;
                emit += sc * uStarBright * 0.6 * uFade * ss / max(sigma + ss, 1e-4) * (ss > 0.0 ? 1.0 : 0.0);
                sigma += ss;
            }

            col += trans * emit * sigma;
            trans *= exp(-sigma * 0.6);
            if (trans < 0.02)
                break;
        }
    }

    col = hdrBoost(col, 3.0); // shell and star only; background and flares are added below

    col += trans * toLinearClamped(starfield(rd) * 0.9);

    // Bloom around the centre (core-bounce flash, remnant neutron star, pre-collapse glow).
    float b2 = length(cross(ro, rd));
    col += vec3(1.0, 0.9, 0.75) * uFlash * 2.5 / (1.0 + b2 * b2 * 0.5);
    col += vec3(0.6, 0.75, 1.0) * uFlash * 0.12 / (1.0 + b2 * b2 * 0.03);
    col += vec3(0.55, 0.75, 1.0) * uRemnant * uFade * 2.0 / (1.0 + b2 * b2 * 25.0);
    col += vec3(1.0, 0.7, 0.4) * uStarBright * 0.03 * uFade / (1.0 + b2 * b2 * 0.4);

    fragColor = vec4(col, 1.0);
}

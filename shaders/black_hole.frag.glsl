#version 330 core

#include "common/camera.glsl"
#include "common/hdr.glsl"
#include "common/stars.glsl"

in vec2 vUV;
out vec4 fragColor;

uniform float uMass;
uniform float uRs;          // Schwarzschild radius, drives light bending
uniform float uHorizon;     // capture radius (Kerr outer horizon)
uniform float uIsco;        // inner disk edge
uniform float uDiskOuter;
uniform float uSpin;
uniform float uDiskTime;
uniform float uDiskBrightness;
uniform float uEscape;      // radius beyond which outgoing rays see the stars

const int MAX_STEPS = 320;
const float BEAM_EXPONENT = 3.5; // Doppler/redshift brightness exponent

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), f.x),
               mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x), f.y);
}

// Turbulence on the disk. Sampling a circle in noise space keeps it seamless in angle.
float diskPattern(float r, float ang) {
    vec2 d = vec2(cos(ang), sin(ang));
    float n = 0.0, amp = 0.5;
    for (int k = 0; k < 4; ++k) {
        float fr = exp2(float(k));
        n += amp * vnoise(vec2(r * 2.5 * fr, 0.0) + d * (1.6 * fr) + float(k) * 13.7);
        amp *= 0.5;
    }
    return n;
}

// Kepler-like angular velocity with a spin correction.
float omega(float r) {
    return sqrt(uMass) / (pow(r, 1.5) + uSpin * pow(uMass, 1.5));
}

// Differential rotation winds the pattern up forever, so cross-fade two
// phase-shifted copies that each reset periodically.
float rotatingPattern(float r, float theta) {
    const float PERIOD = 120.0;
    float w = omega(r);
    float ph = fract(uDiskTime / PERIOD);
    float phB = fract(ph + 0.5);
    float wa = 1.0 - abs(2.0 * ph - 1.0);
    float wb = 1.0 - abs(2.0 * phB - 1.0);
    float ta = ph * PERIOD;
    float tb = phB * PERIOD;
    return wa * diskPattern(r, theta + w * ta) + wb * diskPattern(r, theta + w * tb);
}

vec3 tempColor(float t) {
    vec3 c = mix(vec3(0.9, 0.12, 0.02), vec3(1.0, 0.55, 0.12), smoothstep(0.0, 0.45, t));
    c = mix(c, vec3(1.0, 0.88, 0.6), smoothstep(0.45, 1.1, t));
    return mix(c, vec3(0.8, 0.88, 1.0), smoothstep(1.3, 2.2, t));
}

// Emitted light (premultiplied by opacity in .a-less form) for a ray hitting the disk at P.
vec4 shadeDisk(vec3 P, vec3 rayDir) {
    float r = length(P.xz);
    float theta = atan(P.z, P.x);

    // Orbit direction is (z, 0, -x), i.e. decreasing theta.
    vec3 tangent = normalize(vec3(P.z, 0.0, -P.x));
    float beta = min(sqrt(uMass / r), 0.85);
    float gamma = inversesqrt(1.0 - beta * beta);
    float doppler = 1.0 / (gamma * (1.0 - beta * dot(tangent, -rayDir)));
    float grav = sqrt(max(1.0 - uRs / r, 0.02));
    float g = doppler * grav;

    float temp = pow(uIsco / r, 0.75) * g;
    float pat = 0.25 + 1.5 * rotatingPattern(r, theta);

    float edge = smoothstep(uIsco, uIsco * 1.08, r) * (1.0 - smoothstep(uDiskOuter * 0.7, uDiskOuter, r));
    float alpha = edge * mix(0.95, 0.5, smoothstep(uIsco, uDiskOuter, r));

    vec3 emit = tempColor(temp) * pow(temp, BEAM_EXPONENT) * pat * uDiskBrightness * 2.2;
    return vec4(emit, alpha);
}

void main() {
    vec3 pos = uCamPos;
    vec3 vel = cameraRay(vUV);

    vec3 col = vec3(0.0);
    float trans = 1.0;
    bool captured = false;

    for (int i = 0; i < MAX_STEPS; ++i) {
        float r = length(pos);
        if (r < uHorizon) {
            captured = true;
            break;
        }
        if (r > uEscape && dot(pos, vel) > 0.0)
            break;

        // Null geodesic in a Schwarzschild field: a = -1.5 rs h^2 r_vec / r^5.
        vec3 hv = cross(pos, vel);
        vec3 acc = -1.5 * uRs * dot(hv, hv) * pos / (r * r * r * r * r);

        float dt = 0.04 * r + 0.02;
        vel = normalize(vel + acc * dt);
        vec3 npos = pos + vel * dt;

        if (pos.y * npos.y < 0.0) {
            float f = pos.y / (pos.y - npos.y);
            vec3 P = mix(pos, npos, f);
            float rr = length(P.xz);
            if (rr > uIsco && rr < uDiskOuter) {
                vec4 d = shadeDisk(P, vel);
                col += trans * d.rgb * d.a;
                trans *= 1.0 - d.a;
            }
        }
        pos = npos;
        if (trans < 0.01)
            break;
    }

    col = hdrBoost(col, 2.5); // disk only; stars are added below

    if (!captured && trans >= 0.01)
        col += trans * toLinearClamped(starfield(normalize(vel)));

    fragColor = vec4(col, 1.0);
}

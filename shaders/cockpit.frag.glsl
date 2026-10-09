#version 330 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;
flat in float vMat;
out vec4 fragColor;

uniform vec3 uEventDir; // view-space direction to the event
uniform vec3 uGlow;     // event light on the interior (colour * strength)
uniform float uSpeed;
uniform float uShake;
uniform float uProx;
uniform float uTime;

const vec3 CYAN = vec3(0.15, 0.85, 1.0);
const vec3 AMBER = vec3(1.0, 0.62, 0.15);

float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

float box(vec2 p, vec2 lo, vec2 hi) {
    return step(lo.x, p.x) * step(p.x, hi.x) * step(lo.y, p.y) * step(p.y, hi.y);
}

// Thrust: a stack of bars that fill with speed.
vec3 thrustScreen(vec2 uv) {
    vec3 col = vec3(0.01, 0.03, 0.04);
    float rows = 10.0;
    float row = floor(uv.y * rows);
    float cell = fract(uv.y * rows);
    float lit = step((row + 0.5) / rows, uSpeed);
    float bar = box(vec2(uv.x, cell), vec2(0.12, 0.2), vec2(0.88, 0.8));
    col += mix(CYAN, AMBER, smoothstep(0.6, 0.9, row / rows)) * bar * (0.12 + 1.1 * lit);
    return col;
}

// Nav: a sweep with the event as a blip at its bearing.
vec3 navScreen(vec2 uv) {
    vec3 col = vec3(0.01, 0.03, 0.04);
    vec2 p = (uv - 0.5) * 2.0;
    float r = length(p);
    col += CYAN * 0.5 * smoothstep(0.03, 0.0, abs(r - 0.85));
    col += CYAN * 0.25 * smoothstep(0.03, 0.0, abs(r - 0.45));
    col += CYAN * 0.2 * (smoothstep(0.02, 0.0, abs(p.x)) + smoothstep(0.02, 0.0, abs(p.y))) * step(r, 0.85);
    float a = uTime * 1.3;
    float d = mod(a - atan(p.y, p.x), 6.2831853);
    col += CYAN * 0.5 * exp(-d * 2.2) * step(r, 0.85);
    vec2 blip = clamp(uEventDir.xy / max(length(uEventDir.xy), 1e-3) * min(length(uEventDir.xy) * 4.0, 1.0), -1.0, 1.0) * 0.8;
    col += AMBER * 1.6 * smoothstep(0.1, 0.0, length(p - blip)) * (0.7 + 0.3 * sin(uTime * 6.0));
    return col;
}

// Hull: proximity and shake gauges.
vec3 hullScreen(vec2 uv) {
    vec3 col = vec3(0.01, 0.03, 0.04);
    float flicker = 1.0 - 0.5 * uShake * step(0.8, hash(vec2(floor(uTime * 24.0), floor(uv.y * 14.0))));
    float proxBar = box(uv, vec2(0.18, 0.1), vec2(0.4, 0.1 + 0.8 * uProx));
    float shakeBar = box(uv, vec2(0.6, 0.1), vec2(0.82, 0.1 + 0.8 * uShake));
    col += mix(CYAN, vec3(1.0, 0.2, 0.1), uProx) * proxBar * 1.0 * flicker;
    col += vec3(1.0, 0.35, 0.1) * shakeBar * 1.0 * flicker;
    col += CYAN * 0.12 * (box(uv, vec2(0.18, 0.1), vec2(0.4, 0.9)) + box(uv, vec2(0.6, 0.1), vec2(0.82, 0.9)));
    return col;
}

void main() {
    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing)
        n = -n;
    vec3 v = normalize(-vPos);

    // Strips and screens emit their own light.
    if (vMat > 3.5) {
        fragColor = vec4(CYAN * (0.9 + 0.2 * sin(uTime * 1.7)), 1.0);
        return;
    }
    if (vMat > 0.5) {
        vec3 col = vMat < 1.5 ? thrustScreen(vUV) : (vMat < 2.5 ? navScreen(vUV) : hullScreen(vUV));
        float edge = box(vUV, vec2(0.02), vec2(0.98));
        col *= edge * (0.85 + 0.15 * sin(vUV.y * 220.0 + uTime * 3.0));
        col += CYAN * 0.35 * (1.0 - edge);
        fragColor = vec4(col, 1.0);
        return;
    }

    // Dark metal lit by the event, plus a faint cabin light from above.
    vec3 L = normalize(uEventDir);
    float diff = max(dot(n, L), 0.0);
    float spec = pow(max(dot(reflect(-L, n), v), 0.0), 24.0);
    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    vec3 albedo = vec3(0.05, 0.055, 0.065);
    vec3 col = albedo * (vec3(0.03, 0.04, 0.07) + uGlow * diff + vec3(0.5, 0.6, 0.8) * 0.1 * max(n.y, 0.0));
    col += uGlow * (0.45 * spec + 0.2 * rim);
    fragColor = vec4(col, 1.0);
}

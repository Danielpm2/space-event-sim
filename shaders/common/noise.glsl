// 3D value noise. Include, then call fbm3(p).

float nHash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float vnoise3(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(nHash(i + vec3(0, 0, 0)), nHash(i + vec3(1, 0, 0)), f.x),
                   mix(nHash(i + vec3(0, 1, 0)), nHash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(nHash(i + vec3(0, 0, 1)), nHash(i + vec3(1, 0, 1)), f.x),
                   mix(nHash(i + vec3(0, 1, 1)), nHash(i + vec3(1, 1, 1)), f.x), f.y),
               f.z);
}

float fbm3(vec3 p) {
    float n = 0.0, amp = 0.5;
    for (int i = 0; i < 3; ++i) {
        n += amp * vnoise3(p);
        p = p * 2.03 + vec3(11.7, 3.1, 7.9);
        amp *= 0.5;
    }
    return n / 0.875;
}

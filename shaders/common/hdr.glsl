// HDR helpers. The scene buffer is linear; older shaders authored in display space are decoded here.

vec3 toLinear(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(2.2));
}

// Same look as before: values the old pipeline clipped at 1.0 stay at 1.0.
vec3 toLinearClamped(vec3 c) {
    return toLinear(min(c, vec3(1.0)));
}

// Lifts only the bright part of a linear colour toward `gain` times its value; dim parts are unchanged.
vec3 hdrBoost(vec3 lin, float gain) {
    float m = max(lin.r, max(lin.g, lin.b));
    return lin * (1.0 + (gain - 1.0) * smoothstep(0.5, 1.0, m));
}

// Display-space emissive colour -> linear HDR; highlights exceed 1.0 so they bloom.
vec3 toLinearHdr(vec3 c, float gain) {
    return hdrBoost(toLinearClamped(c), gain);
}

// HDR helpers. The scene buffer is linear; older shaders authored in display space are decoded here.

vec3 toLinear(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(2.2));
}

// Same look as before: values the old pipeline clipped at 1.0 stay at 1.0.
vec3 toLinearClamped(vec3 c) {
    return toLinear(min(c, vec3(1.0)));
}

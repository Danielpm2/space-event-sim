// Starfield sampled by view direction: Milky Way image (when bound by bindSky) plus
// procedural stars. Include, then call starfield(dir).

uniform sampler2D uSky;
uniform float uSkyEnabled;
uniform float uSkyLod;

const float SKY_GAIN = 3.0; // the source image is very dark

float sfHash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

vec3 skyImage(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x) * 0.15915494 + 0.5, asin(clamp(dir.y, -1.0, 1.0)) * 0.31830989 + 0.5);
    return textureLod(uSky, uv, uSkyLod).rgb * SKY_GAIN;
}

vec3 starfield(vec3 dir) {
    vec3 col = vec3(0.0);

    for (int i = 0; i < 3; ++i) {
        float scale = 45.0 + float(i) * 55.0;
        vec3 p = dir * scale;
        vec3 id = floor(p);
        vec3 f = fract(p) - 0.5;
        float h = sfHash(id + float(i) * 11.3);
        if (h > 0.72) {
            vec3 o = vec3(sfHash(id + 1.7), sfHash(id + 4.1), sfHash(id + 9.3)) - 0.5;
            float d = length(f - o * 0.5);
            float size = mix(0.10, 0.22, fract(h * 37.0));
            float star = smoothstep(size, 0.0, d);
            float temp = fract(h * 91.0);
            vec3 tint = mix(vec3(1.0, 0.75, 0.55), vec3(0.65, 0.8, 1.0), temp);
            col += tint * star * (0.5 + 1.5 * fract(h * 53.0));
        }
    }

    if (uSkyEnabled > 0.5) {
        col = col * 0.5 + skyImage(dir);
    } else {
        // Faint galactic band.
        float band = exp(-pow(dot(dir, normalize(vec3(0.3, 1.0, 0.2))) * 3.5, 2.0));
        col += vec3(0.05, 0.05, 0.09) * band;
    }

    return col;
}

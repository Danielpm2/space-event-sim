#version 330 core

// Camera-facing quad from gl_VertexID (draw 4 vertices as a triangle strip).
uniform mat4 uViewProj;
uniform vec3 uCenter;
uniform vec3 uRight;
uniform vec3 uUp;
uniform float uSize;

out vec2 vCorner;

void main() {
    vec2 c = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1)) * 2.0 - 1.0;
    vCorner = c;
    vec3 world = uCenter + (uRight * c.x + uUp * c.y) * uSize;
    gl_Position = uViewProj * vec4(world, 1.0);
}

#version 450

layout(set = 0, binding = 0) uniform MatricesObj {
    mat4 modelViewProjection;
} matrices;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = matrices.modelViewProjection * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
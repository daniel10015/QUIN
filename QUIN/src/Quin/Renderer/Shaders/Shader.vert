#version 450

layout(set = 0, binding = 0) uniform MatricesObj {
    mat4 modelViewProjection;
} matrices;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inNormalCoord;

layout(location = 0) out vec3 fragColor;
//layout(location = 1) out vec4 outColor;
//layout(location = 1) out vec3 fragTexCoordIdx;
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex%3], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex%3];
    
    gl_Position = (matrices.modelViewProjection * vec4(inPosition, 1.0));

    fragColor = inPosition;


    //fragTexCoordIdx = vec3(inTexCoord.xy, inLayer);
}
#version 450

//layout(set = 0, binding = 0) uniform MatricesObj {
//    mat4 modelViewProjection;
//} matrices;

//layout(location = 0) in vec2 inPosition;
//layout(location = 1) in vec4 inColor;
//layout(location = 2) in vec2 inTexCoord;
//layout(location = 3) in float inLayer;

//layout(location = 0) out vec4 fragColor;
//layout(location = 1) out vec3 fragTexCoordIdx;
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //gl_Position = matrices.modelViewProjection * vec4(inPosition, 0.0, 1.0);
    //fragColor = inColor;
    //fragTexCoordIdx = vec3(inTexCoord.xy, inLayer);
}
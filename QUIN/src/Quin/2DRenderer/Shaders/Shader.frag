#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragTexCoordIdx; // (u, v, idx)

layout(location = 0) out vec4 outColor;


void main() {
    outColor = texture(texSampler, fragTexCoordIdx); // load tex into outcolor
    outColor[3] -= fragColor[3]; // alpha
    //outColor *= (fragColor/255.0); // component-wise vec4 multiplication
}
#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragTexCoordIdx; // (u, v, idx)

layout(location = 0) out vec4 outColor;


void main() {
    //outColor = vec4(fragTexCoord, 0.0, 1.0); 
    //outColor = texture(texSampler, fragTexCoord);
    outColor = texture(texSampler, fragTexCoordIdx);
    //outColor = texture(texSampler, vec3(fragTexCoordIdx.xy, 1.0));
    // temporary conditional ... (warp) instead use alpha = 0.0 in the texture
    // if(outColor.w < 1) { discard; }
}
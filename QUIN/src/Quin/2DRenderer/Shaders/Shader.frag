#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    //outColor = vec4(fragTexCoord, 0.0, 1.0); 
    outColor = texture(texSampler, fragTexCoord);
    // temporary conditional ... (warp) instead use alpha = 0.0 in the texture
    // if(outColor.w < 1) { discard; }
}
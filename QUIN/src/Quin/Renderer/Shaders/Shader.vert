#version 450

layout(set = 0, binding = 0) uniform UBOModel { // Use a struct for the model
    mat4 model;
} uboModel;

layout(set = 0, binding = 1) uniform UBOViewProjection { // Use a struct for viewProjection
    mat4 viewProjection;
} uboViewProjection;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inNormalCoord;

layout(location = 0) out vec3 fragColor;

void main() {
    
    // calculate position
    gl_Position = (uboViewProjection.viewProjection * uboModel.model * vec4(inPosition, 1.0));

    // hard code light into shader for now
    vec3 lightPos = vec3(5.0, 20.0, 5.0);
    vec3 color    = vec3(1.0, 1.0, 1.0);

    vec3 fragPosition = (uboModel.model * vec4(inPosition, 1.0)).xyz;
    vec3 normal = normalize(mat3(inverse(uboModel.model)) * inNormal);// normalize(mat3(transpose(inverse(uboModel.model))) * inNormal);

    // Calculate lighting in the vertex shader (Gouraud shading)
    vec3 lightDir = normalize(lightPos - fragPosition);

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    vec3 viewPos = vec3(0.0, 10.0, 15.0);
    // Specular lighting
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);  // Shininess factor
    vec3 specular = spec * color;

    // Combine diffuse and specular lighting
    vec3 resultColor = (diffuse + specular) * vec3(1.0, 0.5, 0.31);  // Object color

    // Output the computed color for interpolation in the fragment shader
    fragColor = resultColor;


    //fragTexCoordIdx = vec3(inTexCoord.xy, inLayer);
}
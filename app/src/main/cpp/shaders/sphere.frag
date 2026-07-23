#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewProjection;
    vec3 cameraPos;
    float time;
    vec3 lightPos;
} ubo;

void main() {
    float ambient = 0.1;

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(ubo.lightPos - vPosition);
    vec3 viewDir = normalize(ubo.cameraPos - vPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 materialColor = vec3(0.8, 0.3, 0.2);
    vec3 lighting = materialColor * (ambient + diff) + vec3(0.3) * spec;

    float posFactor = vPosition.x * 0.1 + vPosition.y * 0.1 + vPosition.z * 0.1;
    vec3 result = lighting;
    result.r *= 0.8 + sin(posFactor) * 0.2;
    result.g *= 0.8 + cos(posFactor) * 0.2;
    result.b *= 0.8 + sin(posFactor * 1.2) * 0.2;

    fragColor = vec4(result, 1.0);
}

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aInstancePos;
layout(location = 3) in vec3 aInstanceParams;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewProjection;
    vec3 cameraPos;
    float time;
    vec3 lightPos;
} ubo;

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec3 vNormal;

void main() {
    float speed = aInstanceParams.x;
    float rotationSpeed = aInstanceParams.y;
    float offset = aInstanceParams.z;

    float timeOffset = ubo.time * speed + offset;
    vec3 animatedPos = aInstancePos;
    animatedPos.x += sin(timeOffset) * 5.0;
    animatedPos.z += cos(timeOffset * 0.7) * 5.0;
    animatedPos.y += sin(timeOffset * 1.3) * 2.0;

    float angle = ubo.time * rotationSpeed + offset * 10.0;
    float sinA = sin(angle);
    float cosA = cos(angle);

    mat3 rotationMatrix = mat3(
        cosA, 0.0, -sinA,
        0.0, 1.0, 0.0,
        sinA, 0.0, cosA
    );

    vec3 rotatedPosition = rotationMatrix * aPosition;
    vec3 rotatedNormal = rotationMatrix * aNormal;

    vec3 worldPos = rotatedPosition + animatedPos;

    gl_Position = ubo.viewProjection * vec4(worldPos, 1.0);
    vPosition = worldPos;
    vNormal = rotatedNormal;
}

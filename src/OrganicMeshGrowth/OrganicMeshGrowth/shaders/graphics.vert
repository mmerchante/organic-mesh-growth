#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	mat4 invViewProj;
	vec3 position;
} camera;

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 rayDirection;
layout(location = 3) out vec3 rayOrigin;


out gl_PerVertex {
    vec4 gl_Position;
};

void main() 
{
	vec4 ndc = vec4(inPosition.xz, .5, 1.0) * 100.0;
	vec4 wsPos = camera.invViewProj * ndc;
	wsPos /= wsPos.w;

	rayDirection = normalize(wsPos.xyz - camera.position);
	rayOrigin = camera.position;

    gl_Position = vec4(inPosition.xz, .5, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

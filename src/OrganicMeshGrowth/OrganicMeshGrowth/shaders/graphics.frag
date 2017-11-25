#version 450
#extension GL_ARB_separate_shader_objects : enable

#define EPSILON 0.001

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler3D sdfSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 rayDirection;
layout(location = 3) in vec3 rayOrigin;

layout(location = 0) out vec4 outColor;

float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
}

float vmin(vec3 v) {
	return min(min(v.x, v.y), v.z);
}

float fBox(vec3 p, vec3 b) {
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}

float sdf(vec3 pos)
{
	pos += .5;
	float dist = texture(sdfSampler, pos).x;
	return dist;
}

vec3 sdfNormal(vec3 pos, float epsilon)
{
	vec2 eps = vec2(epsilon, 0.0);

	float dx = sdf(pos + eps.xyy) - sdf(pos - eps.xyy);
	float dy = sdf(pos + eps.yxy) - sdf(pos - eps.yxy);
	float dz = sdf(pos + eps.yyx) - sdf(pos - eps.yyx);

	return normalize(vec3(dx, dy, dz));
}

vec3 sdf_viz(vec3 rO, vec3 rD)
{
	rO.y += .5;
    float t = -rO.y / rD.y;
        
    if(t < 0.0)
        return vec3(0.0);
    
    vec3 p = rO + rD * t;    

	if(abs(p.x) > .5 || abs(p.z) > .5)
		return vec3(0.0);

    float d = sdf(p) * 3.0;
    return vec3(smoothstep(.1, .2, mod(d, 1.0)) * .5);
}


void main() 
{
	float t = 0.0;
	bool hit = false;

	//outColor = vec4(sdf_viz(rayOrigin, rayDirection), 1.0);

	for(int i = 0; i < 150; ++i)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		float dist = sdf(pos) * .05;

		t += dist;

		if(dist < EPSILON)
		{
			hit = true;
			break;
		}

	}

	if(hit)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		vec3 normal = sdfNormal(pos, EPSILON);
		outColor = vec4(dot(normal, vec3(.577))) * .75 + .25;
	}
	else
	{
	    outColor = vec4(sdf_viz(rayOrigin, rayDirection), 1.0);
	}
}

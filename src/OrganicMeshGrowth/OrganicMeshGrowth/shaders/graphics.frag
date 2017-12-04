#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_DISTANCE 1.7320508
#define EPSILON 0.002

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	mat4 invViewProj;
	vec3 position;
} camera;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler3D sdfSampler;
layout(set = 1, binding = 3) uniform sampler3D vectorFieldSampler;

layout(location = 0) in vec3 rayOrigin;

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
	return max(0.0, dist);
}

vec3 sdfNormal(vec3 pos, float epsilon)
{
	vec2 eps = vec2(epsilon, 0.0);

	float dx = sdf(pos + eps.xyy) - sdf(pos - eps.xyy);
	float dy = sdf(pos + eps.yxy) - sdf(pos - eps.yxy);
	float dz = sdf(pos + eps.yyx) - sdf(pos - eps.yyx);

	return normalize(vec3(dx, dy, dz));
}

const vec3 CLEAR_COLOR = vec3(.1, .09, .1);

vec3 sdf_viz(vec3 rO, vec3 rD)
{
	rO.y += .5;
    float t = -rO.y / rD.y;
        
    if(t < 0.0)
        return CLEAR_COLOR;
    
    vec3 p = rO + rD * t;    

	if(abs(p.x) > .5 || abs(p.z) > .5)
		return CLEAR_COLOR;

	p.y += .5;
    float d = sdf(p) * 24.0;
    return mix(CLEAR_COLOR, CLEAR_COLOR * 2.0, (smoothstep(.1, .2, mod(d, 1.0)) * .5));
}

#define AO_ITERATIONS 15
#define AO_DELTA .0075
#define AO_DECAY .9
#define AO_INTENSITY 1.0

float evaluateAmbientOcclusion(vec3 point, vec3 normal)
{
	float ao = 0.0;
	float delta = AO_DELTA;
	float decay = 1.0;

	for(int i = 0; i < AO_ITERATIONS; i++)
	{
		float d = float(i) * delta;
		decay *= AO_DECAY;
		ao += (d - max(0.0, sdf(point + normal * d))) / decay;
	}

	return clamp(1.0 - ao * AO_INTENSITY, 0.0, 1.0);
}

//Curvature in 7-tap (more accurate)
float curv2(in vec3 p, in float w)
{
    vec3 e = vec3(w, 0, 0);
    
    float t1 = sdf(p + e.xyy), t2 = sdf(p - e.xyy);
    float t3 = sdf(p + e.yxy), t4 = sdf(p - e.yxy);
    float t5 = sdf(p + e.yyx), t6 = sdf(p - e.yyx);
    
    return .25/e.x*(t1 + t2 + t3 + t4 + t5 + t6 - 6.0*sdf(p));
}
void main() 
{
	vec3 rayDirection = normalize(rayOrigin - camera.position);
	float t = 0.0;
	bool hit = false;
	float dist = 100.0;

	for(int i = 0; i < 1500.0; ++i)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		dist = sdf(pos);// min(0.05, sdf(pos) * .05);

		t += dist * .02;//clamp(dist * .05, 0.0, .001);

		if(dist < EPSILON)
		{
			hit = true;
			break;
		}

		// A bit expensive but eh
		if(vmax(abs(pos)) > .5 + EPSILON)
			break;
	}

	if(hit)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		vec3 normal = sdfNormal(pos, EPSILON);

		// Sphere lit
		vec3 ssNormal = (camera.view * vec4(normal, 0.0)).xyz * vec3(1.0, -1.0, 1.0) * .5 + .5;
		outColor = texture(texSampler, ssNormal.xy);
		//outColor = vec4(curv2(pos, .1)) * .5;

		//outColor = texture(vectorFieldSampler, pos * .5 + .5);
	}
	else
	{
	    outColor = vec4(sdf_viz(rayOrigin, rayDirection), 1.0);
	}
}

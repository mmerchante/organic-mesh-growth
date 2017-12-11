#version 450
#extension GL_ARB_separate_shader_objects : enable

//#define VISUALIZE_SDF
#define DEBUG_SDF
#define MAX_DISTANCE 1.7320508
#define EPSILON 0.0035

#define MAX_ITERATIONS 350

#define saturate(x) clamp(x, 0.0, 1.0)

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	mat4 invViewProj;
	vec3 position;
} camera;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler3D sdfSampler;
layout(set = 1, binding = 3) uniform sampler3D vectorFieldSampler;

layout(set = 1, binding = 4) uniform TimeBufferObject {
    float deltaTime;
    float totalTime;
	float simulationDeltaTime;
} time;

layout(location = 0) in vec3 rayOrigin;

layout(location = 0) out vec4 outColor;

float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
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
#ifdef DEBUG_SDF
	float offset = sin(time.totalTime * .1) * .5 + .5;
#else
	float offset = 0.0;
#endif

	rO.y += .5 - offset;
    float t = -rO.y / rD.y;
        
    if(t < 0.0)
        return CLEAR_COLOR;
    
    vec3 p = rO + rD * t;    

	if(abs(p.x) > .5 || abs(p.z) > .5)
		return CLEAR_COLOR;

	p.y = offset - .5;
    float d = sdf(p) * 12.0;
    return mix(CLEAR_COLOR * 8.0, CLEAR_COLOR, (smoothstep(.1, .11, mod(d, 1.0))));
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

float curvature(in vec3 p, in float w)
{
    vec2 e = vec2(w, 0);
    
    float t1 = sdf(p + e.xyy), t2 = sdf(p - e.xyy);
    float t3 = sdf(p + e.yxy), t4 = sdf(p - e.yxy);
    float t5 = sdf(p + e.yyx), t6 = sdf(p - e.yyx);
    
    return saturate((.25/e.x) * (t1 + t2 + t3 + t4 + t5 + t6 - (6.0 * sdf(p))));
}

void main() 
{
	vec3 rayDirection = normalize(rayOrigin - camera.position);
	float t = 0.0;
	bool hit = false;
	float dist = 100.0;

	vec3 resultColor;

	for(int i = 0; i < MAX_ITERATIONS; ++i)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		dist = sdf(pos);

		if(dist < EPSILON)
		{
			hit = true;
			break;
		}

		t += dist * .035;//clamp(dist * .02, 0.0, .001);

		// A bit expensive but eh
		if(vmax(abs(pos)) > .501 + EPSILON)
			break;
	}

	t -= EPSILON;

	if(hit)
	{
		vec3 pos = rayOrigin + rayDirection * t;
		vec3 normal = sdfNormal(pos, EPSILON);
		vec3 lightPosition = vec3(0.6, 1.0, 1.2);
		vec3 lightDirection = normalize(lightPosition - pos);
		float lightIntensity = 2.0;

		float c = smoothstep(0.0, 1.0, 1.0 - saturate(curvature(pos, .05)));

		float diffuse = sdf(pos - rayDirection * .05) / .4;
		float sss = saturate((sdf(pos + normal * .01 + lightDirection * .05) ) / .175);
		sss = smoothstep(0.0, 1.0, sss);

		vec3 H = normalize(lightDirection - rayDirection);
		float specular = pow(abs(dot(H, normal)), 25.5);

		// Make sure the rim is smaller on shadow
		float facingRatio = pow(1.0 - max(0.0, dot(normal, -rayDirection)), 2.0) * mix(.3, 1.0, sss);

		vec3 baseColor = vec3(.9, .5, .1);
		vec3 envColor = vec3(.6, .8, .8);
		vec3 coreColor = pow(baseColor, vec3(3.0));//vec3(1.0, .3, .01);
		vec3 specularColor = vec3(.4, .7, .9);
		vec3 ambient = envColor * envColor * .05 + coreColor * .1;

		resultColor = mix(baseColor, coreColor * coreColor, saturate(c + 1.0 - sss)) * (sss + diffuse * .2) * .5 * lightIntensity;
		resultColor += specularColor * (specular * .3) + envColor * facingRatio * .45;
		resultColor += ambient;
	}
	else
	{
		#ifdef VISUALIZE_SDF
	    resultColor = sdf_viz(rayOrigin, rayDirection);
		#else 
		resultColor = CLEAR_COLOR;
		#endif
	}

	outColor = vec4(resultColor, 1.0);
}

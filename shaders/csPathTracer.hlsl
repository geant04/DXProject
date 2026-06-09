RWTexture2D<float4> outputTexture : register(u0);
cbuffer CameraConstants : register(b0)
{
	float4x4 viewProjInverse;
	float4 cameraPosition;
	uint frameNumber;
}

#include "Common.hlsli"

// Scene details
static const Sphere sphere = 
{ 
	float3(0.0f, 5.0f, 0.0f), 
	1.0f, 
	{ 
		float3(0.88f, 0.33f, 0.33f),
		0.0f,
		0.0f,
		false
	} 
};
static const Sphere metalSphere =
{
	float3(1.8f, 5.0f, -0.25f),
	0.75f,
	{
		float3(0.33f, 0.88f, 0.33f),
		1.0f, // metal
		0.0f,
		false
	}
};
static const Sphere glassSphere =
{
	float3(-1.8f, 5.0f, -0.25f),
	0.75f,
	{
		float3(1.0f, 1.0f, 1.0f),
		0.0f,
		1.0f, // transmissive
		false
	}
};
static const Plane groundPlane = 
{ 
	float3(0.0f, 0.0f, -1.0f), 
	float3(0.0f, 0.0f, 1.0f), 
	float2(0.0f, 0.0f), 
	{ 
		float3(0.88f, 0.88f, 0.88f), 
		0.0f,
		0.0f,
		false 
	}
};
static const Plane wallPlane1 =
{
	float3(-4.0f, 10.0f, -1.0f),
	float3(1.0f, 0.0f, 0.0f),
	float2(0.0f, 0.0f),
	{
		float3(0.88f, 0.33f, 0.33f),
		0.0f,
		0.0f,
		false
	}
};
static const Plane wallPlane2 =
{
	float3(4.0f, 10.0f, -1.0f),
	float3(-1.0f, 0.0f, 0.0f),
	float2(0.0f, 0.0f),
	{
		float3(0.33f, 0.88f, 0.33f),
		0.0f,
		0.0f,
		false
	}
};
static const Plane wallPlane3 =
{
	float3(0.0f, 10.0f, -1.0f),
	float3(0.0f, -1.0f, 0.0f),
	float2(0.0f, 0.0f),
	{
		float3(0.88f, 0.88f, 0.88f),
		0.0f,
		0.0f,
		false
	}
};
static const Plane lightPlane =
{
	float3(0.0f, 4.0f, 8.0f),
	normalize(float3(0.0f, 1.0f, -1.0f)),
	float2(7.0f, 7.0f),
	{
		float3(2.4f, 2.4f, 2.4f),
		0.0f,
		0.0f,
		true
	}
};

static const float3 lightPosition = float3(0.0, 2.0, 5.0f);

// Rendering details
static const uint NUM_ITERATIONS = 10;

void GetIntersection(in Ray ray, inout Intersect intersect)
{
	intersect.t = 100000.0f;
	intersect.isHit = false;
	
	// Will expand upon this later when we have more geometry with more complex intersection tests.
	GetPlaneIntersection(groundPlane, ray, intersect);
	GetPlaneIntersection(wallPlane1, ray, intersect);
	GetPlaneIntersection(wallPlane2, ray, intersect);
	GetPlaneIntersection(wallPlane3, ray, intersect);
	GetPlaneIntersection(lightPlane, ray, intersect);
	GetSphereIntersection(sphere, ray, intersect);
	GetSphereIntersection(metalSphere, ray, intersect);
	GetSphereIntersection(glassSphere, ray, intersect);
}

float3 GetDiffuseWi(in float3 worldNormal, uint2 seed)
{
	float randR = rng(seed);
	float randG = rng(seed + uint2(1, 1));
	
	float2 randXY = 2.0f * float2(randR, randG) - 1.0f;
	
	// Get tangent and bitangent vectors from worldNormal
	float3 referenceUp = abs(worldNormal.z) > 0.999 ? float3(0.0f, 1.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f);
	float3 tangent = normalize(cross(referenceUp, worldNormal));
	float3 bitangent = cross(worldNormal, tangent);

	return normalize(worldNormal + tangent * randXY.x + bitangent * randXY.y);
}

// inDir vector points "outwards", if that makes sense.
float3 GetWi(in float3 worldInDir, in float3 worldNormal, in Material material, uint2 seed)
{
	if (material.metallic > 0.0f)
	{
		// Reflect doesn't "reflect" in the way that the in-vector points outwards.
		return normalize(reflect(-worldInDir, worldNormal));
	}
	if (material.transmissive > 0.0f)
	{
		// TODO: Need to fix the sphere intersection test to account for ray-origin inside the sphere.
		float etaA = 1.0f;
		float etaB = 1.55f;
		float eta = dot(worldInDir, worldNormal) < 0.0f ? etaB / etaA : etaA / etaB;
		return normalize(refract(worldInDir, worldNormal, eta));
	}
	
	// Will add more complicated BSDFs later.
	// For now, treat all materials as a diffuse material.
	return GetDiffuseWi(worldNormal, seed);
}

float3 NaivePathTracer(in Ray ray, uint iterations, uint2 id)
{
	float3 outColor = 1.0f;
	Intersect intersect;
	
	for (uint i = 1; i <= iterations; i++)
	{
		GetIntersection(ray, intersect);
		
		// Hit nothing - i.e the sky
		if (!intersect.isHit)
		{
			return 0.0f;
		}
		
		// If we hit a light source, return.
		if (intersect.material.isEmissive)
		{
			return intersect.material.color * outColor;
		}
		
		float3 normal = intersect.normal;
		float3 bsdfColor = intersect.material.color;
		float3 sampleDirection = GetWi(-ray.direction, normal, intersect.material, id * i);
		
		float lambert = max(dot(normal, sampleDirection), 0.0f);
		
		if (intersect.material.metallic > 0.0f || intersect.material.transmissive > 0.0f)
		{
			lambert = 1.0f;
		}
		
		outColor *= bsdfColor * lambert;
		ray.direction = sampleDirection;
		ray.origin = intersect.position + ray.direction * 0.00001f;
	}
	
	// Return nothing, since we didn't hit a light.
	return 0.0f;
}

float3 NaiveRayTracer(in Ray ray, in float3 animatedLightPosition)
{
	float3 outColor = 0.0f;
	
	Intersect intersect;
	intersect.t = 100000.0f;
	intersect.isHit = false;
	
	GetIntersection(ray, intersect);
	
	if (intersect.isHit)
	{
		// Lighting!
		float3 normal = intersect.normal;
		float3 color = intersect.material.color;
		
		float3 intersectToLight = animatedLightPosition - intersect.position;
		float lightDistanceSq = dot(intersectToLight, intersectToLight);
		float lightPower = 40.0f;
		
		float3 light = normalize(intersectToLight);
		float lambert = max(dot(light, normal), 0.0f);
		
		// Shadow Test
		Intersect shadowIntersect;
		shadowIntersect.t = 100000.0f;
		shadowIntersect.isHit = false;
		
		Ray shadowRay;
		shadowRay.direction = light;
		shadowRay.origin = intersect.position + shadowRay.direction * 0.001f;
		
		GetIntersection(shadowRay, shadowIntersect);
		float shadow = 1.0f;
		
		if (shadowIntersect.isHit && shadowIntersect.t * shadowIntersect.t < lightDistanceSq)
		{
			shadow = 0.0f;
		}
		
		color *= lambert * shadow * (lightPower / lightDistanceSq);
		
		outColor = color;
	}
	
	return outColor;
}

[numthreads(8, 8, 1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{	
	// AA jitter hack
	float2 jitter = float2(rng(id.xy * (frameNumber + 1)), rng(id.xy * (frameNumber + 1) * 31));
	
	float2 uv = (float2(id.x, id.y) + 2.0f * jitter) / float2(1280.0f, 720.0f);
	uv.y = 1.0f - uv.y;
	
	float2 ndc = 2.0f * uv - 1.0f;
	float aspectRatio = 720.0f / 1280.0f;
	
	// Let our coordinate space be Z-up... I guess
	float3 direction = normalize(float3(ndc.x, 1.0f, ndc.y * aspectRatio));
	float3 origin = float3(0.0f, 0.0f, 0.5f);
	
	float3 animatedLightPosition = lightPosition;
	
	// Ray-tracing setup...
	Ray ray;
	ray.direction = direction;
	ray.origin = origin;
	
	Intersect intersect;
	intersect.t = 100000.0f;
	intersect.isHit = false;
	
	//float3 outColor = NaiveRayTracer(ray, animatedLightPosition);
	float3 outColor = NaivePathTracer(ray, NUM_ITERATIONS, id.xy * frameNumber);
	
	// Lane-independent smoothing call
	float3 accumulatedColor = outputTexture[id.xy].rgb;
	outColor = lerp(accumulatedColor, outColor, 1.0f / (frameNumber));
	
	// float randR = rng(id.xy * frameNumber);
	outputTexture[id.xy] = float4(outColor, 1);
}
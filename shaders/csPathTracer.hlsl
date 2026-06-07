RWTexture2D<float4> outputTexture : register(u0);

// #include "Intersections.hlsl"

struct Material
{
	float3 color;
};

struct Sphere
{
	float3 center;
	float radius;
	Material material;
};

struct Plane
{
	float3 center;
	float3 normal;
	float2 scaleXY;
	Material material;
};

struct Intersect
{
	float3 position;
	float3 normal;
	Material material;
	float t;
	bool isHit;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

static const Sphere sphere = { float3(0.0f, 5.0f, 0.0f), 1.0f, { float3(0.88f, 0.33f, 0.33f) } };
static const Plane groundPlane = { float3(0.0f, 0.0f, -1.0f), float3(0.0f, 0.0f, 1.0f), float2(1.0f, 1.0f), { float3(0.88f, 0.88f, 0.88f) } };
static const float3 lightPosition = float3(0.0, 2.0, 5.0f);

void GetSphereIntersection(in Sphere sphere, in Ray ray, inout Intersect intersect)
{	
	float3 l = sphere.center - ray.origin;
	float lengthSq = dot(l, l);
	float projLOnDir = dot(l, ray.direction);
	
	if (projLOnDir < 0.0f) return;
	
	float distanceToTangentSq = lengthSq - (projLOnDir * projLOnDir);
	float radiusSq = sphere.radius * sphere.radius;
	if (distanceToTangentSq > radiusSq) return;
	
	float t1 = projLOnDir - sqrt(radiusSq - distanceToTangentSq);
	
	if (intersect.t > t1)
	{
		intersect.isHit = true;
		intersect.t = t1;
		intersect.position = ray.origin + ray.direction * t1;
		intersect.normal = normalize(intersect.position - sphere.center);
		intersect.material = sphere.material;
	}
}
	
void GetPlaneIntersection(in Plane plane, in Ray ray, inout Intersect intersect)
{
	// p = ray * t + ray.origin - plane.center , s.t dot(p, plane.normal) = 0
	// dot(ray * t, plane.normal) + dot(ray.origin, plane.normal) - dot(plane.center, plane.normal) = 0
	// t * dot(ray, plane.normal) + dot(ray.origin, plane.normal) - dot(plane.center, plane.normal) = 0
	// t = dot(plane.normal, plane.center - ray.origin) / dot(ray, plane.normal)
	
	float rayDirDotNormal = dot(ray.direction, plane.normal);
	
	if (abs(rayDirDotNormal) < 0.001f)
	{
		return;
	}
	
	float t = dot(plane.normal, plane.center - ray.origin) / rayDirDotNormal;
	
	if (t <= 0)
	{
		return;
	}
	
	if (intersect.t > t)
	{
		intersect.isHit = true;
		intersect.t = t;
		intersect.position = ray.origin + ray.direction * t;
		intersect.normal = plane.normal;
		intersect.material = plane.material;
	}
}

void GetIntersection(in Ray ray, inout Intersect intersect)
{
	// Will expand upon this later when we have more geometry with more complex intersection tests.
	GetPlaneIntersection(groundPlane, ray, intersect);
	GetSphereIntersection(sphere, ray, intersect);
}

void GetShadow(in Ray ray, inout Intersect intersect)
{
	GetIntersection(ray, intersect);
}


[numthreads(8, 8, 1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{
	float2 uv = (float2(id.x, id.y) + 0.5f) / float2(1280.0f, 720.0f);
	uv.y = 1.0f - uv.y;
	
	float2 ndc = 2.0f * uv - 1.0f;
	float aspectRatio = 720.0f / 1280.0f;
	
	// Let our coordinate space be Z-up... I guess
	float3 direction = normalize(float3(ndc.x, 1.0f, ndc.y * aspectRatio));
	float3 origin = float3(0.0f, 0.0f, 0.5f);
	
	Ray ray;
	ray.direction = direction;
	ray.origin = origin;
	
	Intersect intersect;
	intersect.t = 100000.0f;
	intersect.isHit = false;
	GetIntersection(ray, intersect);
	
	if (intersect.isHit)
	{
		// Lighting!
		float3 normal = intersect.normal;
		float3 color = intersect.material.color;
		
		float3 intersectToLight = lightPosition - intersect.position;
		float lightDistanceSq = dot(intersectToLight, intersectToLight);
		float lightPower = 40.0f;
		
		float3 light = normalize(lightPosition - intersect.position);
		float lambert = dot(light, normal);
		
		// Shadow Test
		Intersect shadowIntersect;
		shadowIntersect.t = 100000.0f;
		shadowIntersect.isHit = false;
		
		Ray shadowRay;
		shadowRay.direction = light;
		shadowRay.origin = intersect.position + shadowRay.direction * 0.001f;
		
		GetShadow(shadowRay, shadowIntersect);
		color *= lambert * !shadowIntersect.isHit * (lightPower / lightDistanceSq);
		
		outputTexture[id.xy] = float4(color, 1);
	}
	else
	{
		outputTexture[id.xy] = float4(0.0f, 0.0f, 0.0f, 1);
	}
}
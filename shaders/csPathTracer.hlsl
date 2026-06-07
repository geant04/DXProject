RWTexture2D<float4> outputTexture : register(u0);

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

static const Sphere sphere = { float3(0.0f, 5.0f, 0.0f), 1.0f, { float3(1.0f, 0.0f, 0.0f) } };

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
	
	intersect.isHit = true;
	intersect.t = t1;
	intersect.position = ray.origin + ray.direction * t1;
	intersect.normal = normalize(intersect.position - sphere.center);
	intersect.material = sphere.material;
}

void GetIntersection(in Ray ray, inout Intersect intersect)
{
	// Will expand upon this later when we have more geometry with more complex intersection tests.
	GetSphereIntersection(sphere, ray, intersect);
}


[numthreads(8, 8, 1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{
	float2 uv = (float2(id.xy) + 0.5f) / float2(1280.0f, 720.0f);
	float2 ndc = 2.0f * uv - 1.0f;
	float aspectRatio = 720.0f / 1280.0f;
	
	// Let our coordinate space be Z-up... I guess
	float3 direction = normalize(float3(ndc.x, 1.0f, ndc.y * aspectRatio));
	float3 origin = float3(0.0f, 0.0f, 0.0f);
	
	Ray ray;
	ray.direction = direction;
	ray.origin = origin;
	
	Intersect intersect;
	intersect.isHit = false;
	GetIntersection(ray, intersect);
	
	if (intersect.isHit)
	{
		float3 mappedNormal = 0.5f * intersect.normal + 0.5f;
		outputTexture[id.xy] = float4(mappedNormal, 1);
	}
	else
	{
		outputTexture[id.xy] = float4(0.0f, 0.0f, 0.0f, 1);
	}
}
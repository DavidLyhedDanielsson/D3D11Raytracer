#include "Constants.hlsl"

//RWTexture2D<float4> output : register(u0);
RWTexture2D<float4> rayPositions : register(u0);
RWTexture2D<float4> rayDirections : register(u1);

cbuffer sphereBuffer : register(b0)
{
	float4 spheres[MAX_SPHERES];
	int sphereCount;
};

cbuffer triangleBuffer : register(b1)
{
	float4 vertices[MAX_TRIANGLES * 3];
	int triangleCount;
};

void SphereTrace(float3 rayPosition, float3 rayDirection, inout float depth, inout float3 currentNormal);
void TriangleTrace(float3 rayPosition, float3 rayDirection, inout float depth, inout float3 currentNormal);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float2 texCoords = threadID.xy / float2(1280.0f, 720.0f);

	float4 rayPositionAndDepth = rayPositions[threadID.xy];
	float3 rayDirection = rayDirections[threadID.xy].xyz;
	
	float3 normal = float3(0.0f, 0.0f, 0.0f);

	SphereTrace(rayPositionAndDepth.xyz, rayDirection, rayPositionAndDepth.w, normal);
	TriangleTrace(rayPositionAndDepth.xyz, rayDirection, rayPositionAndDepth.w, normal);

	if(dot(normal, normal) == 0.0f)
		return;

	float lightFac = dot(normalize(normal), LIGHT_DIR);

	//output[threadID.xy] = float4(lightFac, lightFac, lightFac, 1.0f);
	rayPositions[threadID.xy] = float4(rayPositionAndDepth.xyz + rayDirection * rayPositionAndDepth.w, rayPositionAndDepth.w);
}

void SphereTrace(float3 rayPosition, float3 rayDirection, inout float depth, inout float3 currentNormal)
{
	int closestSphereIndex = -1;

	for(int i = 0; i < sphereCount; ++i)
	{
		float3 spherePosition = spheres[i].xyz;
		float sphereRadius = spheres[i].w;

		float a = dot(rayDirection, (rayPosition - spherePosition));

		float3 dirToSphere = rayPosition - spherePosition;
		float b = dot(dirToSphere, dirToSphere);

		float root = (a * a) - b + (sphereRadius * sphereRadius);

		if(root < 0.0f)
			continue;

		float distance = -a - sqrt(root);

		if(distance < depth
			&& distance >= 0.0f)
		{
			closestSphereIndex = i;
			depth = distance; //TODO: Local variable?
		}
	}

	if(closestSphereIndex == -1)
		return;

	float3 hitPoint = rayPosition + rayDirection * depth;
	float3 normal = hitPoint - spheres[closestSphereIndex].xyz;

	currentNormal = normalize(normal);
}

void TriangleTrace(float3 rayPosition, float3 rayDirection, inout float depth, inout float3 currentNormal)
{
	int closestTriangleIndex = -1;

	for(int i = 0; i < triangleCount; ++i)
	{
		float3 v0 = vertices[i * 3].xyz;
		float3 v1 = vertices[i * 3 + 1].xyz;
		float3 v2 = vertices[i * 3 + 2].xyz;

		float3 v0v2 = v2 - v0;
		float3 v0v1 = v1 - v0;
		float3 v0ray = rayPosition - v0;

		float prediv = 1.0f / dot(v0v1, cross(rayDirection, v0v2));
		float3 precross = cross(v0ray, v0v1);

		float distance = dot(v0v2, precross) * prediv;
		float x = dot(v0ray, cross(rayDirection, v0v2)) * prediv;
		float y = dot(rayDirection, precross) * prediv;

		if(distance >= 0.0f && x >= 0.0f && y >= 0.0f && x + y <= 1.0f
			&& distance < depth)
		{
			closestTriangleIndex = i;
			depth = distance;
		}
	}

	if(closestTriangleIndex == -1)
		return;

	float3 v0 = vertices[closestTriangleIndex * 3].xyz;
	float3 v1 = vertices[closestTriangleIndex * 3 + 1].xyz;
	float3 v2 = vertices[closestTriangleIndex * 3 + 2].xyz;

	currentNormal = normalize(cross(v2 - v0, v1 - v0));
}
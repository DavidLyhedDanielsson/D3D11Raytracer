#include "Constants.hlsl"

RWTexture2D<float4> rayPositionsOut : register(u0);
RWTexture2D<float4> rayDirectionsOut : register(u1);
RWTexture2D<float4> rayNormalOut : register(u2);
RWTexture2D<float4> rayColorOut : register(u3);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

sampler textureSampler : register(s0);

cbuffer sphereBuffer : register(b0)
{
	float4 spheres[MAX_SPHERES];
	float4 sphereColors[MAX_SPHERES];
	int sphereCount;
};

cbuffer triangleBuffer : register(b1)
{
	float4 vertices[MAX_VERTICES];
	int4 indicies[MAX_INDICIES / 3];
	float4 triangleColors[MAX_INDICIES / 3];
	int triangleCount;
};

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);
void TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float4 rayPosition = rayPositions[threadID.xy];
	float3 rayDirection = rayDirections[threadID.xy].xyz;
	
	float3 normal = float3(0.0f, 0.0f, 0.0f);
	float3 color;

	int closestSphere = -1;
	int closestTriangle = -1;

	float depth = FLOAT_MAX;

	int lastHit = rayPosition.w;

	SphereTrace(rayPosition.xyz, rayDirection, lastHit, depth, normal, closestSphere);
	TriangleTrace(rayPosition.xyz, rayDirection, lastHit, depth, normal, closestTriangle);
	
	if(dot(normal, normal) == 0.0f)
	{
		rayNormalOut[threadID.xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}

	float3 outDirection = reflect(rayDirection, normal);
	rayDirectionsOut[threadID.xy] = float4(outDirection, 0.0f);
	rayNormalOut[threadID.xy] = float4(normal, 0.0f);

	if(closestTriangle != -1)
	{
		//A triangle was closest

		rayColorOut[threadID.xy] = triangleColors[closestTriangle];
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, sphereCount + closestTriangle);
	}
	else
	{
		//A sphere was closest

		rayColorOut[threadID.xy] = sphereColors[closestSphere];
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, closestSphere);
	}
}

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
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
			&& distance > 0.0f
			&& i != lastHit)
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

	closestIndex = closestSphereIndex;
}

void TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestTriangleIndex = -1;

	for(int i = 0; i < triangleCount; ++i)
	{
		float3 v0 = vertices[indicies[i].x].xyz;
		float3 v1 = vertices[indicies[i].y].xyz;
		float3 v2 = vertices[indicies[i].z].xyz;

		float3 v0v2 = v2 - v0;
		float3 v0v1 = v1 - v0;
		float3 v0ray = rayPosition - v0;

		float prediv = 1.0f / dot(v0v1, cross(rayDirection, v0v2));
		float3 precross = cross(v0ray, v0v1);

		float distance = dot(v0v2, precross) * prediv;
		float x = dot(v0ray, cross(rayDirection, v0v2)) * prediv;
		float y = dot(rayDirection, precross) * prediv;

		if(distance > 0.0f && x >= 0.0f && y >= 0.0f && x + y <= 1.0f
			&& distance < depth
			&& sphereCount + i != lastHit)
		{
			closestTriangleIndex = i;
			depth = distance;
		}
	}

	if(closestTriangleIndex == -1)
		return;

	float3 v0 = vertices[indicies[closestTriangleIndex].x].xyz;
	float3 v1 = vertices[indicies[closestTriangleIndex].y].xyz;
	float3 v2 = vertices[indicies[closestTriangleIndex].z].xyz;

	currentNormal = normalize(cross(v2 - v0, v1 - v0));

	closestIndex = closestTriangleIndex;
}
#include "SuperSampledSharedBuffers.h"

RWTexture2D<float4> rayPositionsOut : register(u0);
RWTexture2D<float4> rayDirectionsOut : register(u1);
RWTexture2D<float4> rayNormalOut : register(u2);
RWTexture2D<float4> rayColorOut : register(u3);
RWTexture2D<float> depthOut : register(u4);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

Texture2D diffuseTexture : register(t2);

sampler textureSampler : register(s0);

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);
float2 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);

float4 GetTriangleColorAt(int triangleIndex, float2 barycentricCoordinates);

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
	float2 barycentric = TriangleTrace(rayPosition.xyz, rayDirection, lastHit, depth, normal, closestTriangle);
	
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

		uint sphereCount = 0;
		uint stride = 0;

		spheres.GetDimensions(sphereCount, stride);

		rayColorOut[threadID.xy] = GetTriangleColorAt(closestTriangle, barycentric);
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, sphereCount + closestTriangle);
	}
	else
	{
		//A sphere was closest

		rayColorOut[threadID.xy] = spheres[closestSphere].color;
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, closestSphere);
	}

	depthOut[threadID.xy] = depth;
}

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestSphereIndex = -1;

	uint sphereCount = 0;
	uint stride = 0;

	spheres.GetDimensions(sphereCount, stride);

	for(int i = 0; i < (int)sphereCount; ++i)
	{
		float3 spherePosition = spheres[i].position.xyz;
		float sphereRadius = spheres[i].position.w;

		float distance = 0.0f;

		if(!RaySphereIntersection(rayPosition, rayDirection, spherePosition, sphereRadius, distance))
			continue;

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
	float3 normal = hitPoint - spheres[closestSphereIndex].position.xyz;

	currentNormal = normalize(normal);

	closestIndex = closestSphereIndex;
}

float2 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestTriangleIndex = -1;

	float u = 0.0f;
	float v = 0.0f;

	float3 closestV0 = float3(1.0f, 0.0f, 0.0f);
	float3 closestV1 = float3(0.0f, 1.0f, 0.0f);
	float3 closestV2 = float3(0.0f, 0.0f, 1.0f);

	uint sphereCount = 0;
	uint stride = 0;

	spheres.GetDimensions(sphereCount, stride);

	uint modelCount = 0;

	models.GetDimensions(modelCount, stride);

	for(int i = 0; i < (int)modelCount; ++i)
	{
		float3 aabbMin = models[i].aabb.min;
		float3 aabbMax = models[i].aabb.max;

		if(RayAABBIntersection(rayPosition, rayDirection, aabbMin, aabbMax))
		{
			int beginIndex = models[i].beginIndex;
			int endIndex = models[i].endIndex;

			for(int ii = beginIndex; ii < endIndex; ++ii)
			{
				float3 v0 = vertices[triangles[ii].indicies.x].position.xyz;
				float3 v1 = vertices[triangles[ii].indicies.y].position.xyz;
				float3 v2 = vertices[triangles[ii].indicies.z].position.xyz;			

				float tempU = 0.0f;
				float tempV = 0.0f;

				float t = 0.0f;

				if(!RayTriangleIntersection(rayPosition, rayDirection, v0, v1, v2, tempU, tempV, t))
					continue;

				if(t > 0.0f 
					&& t < depth
					&& ii + sphereCount != lastHit)
				{
					u = tempU;
					v = tempV;

					closestTriangleIndex = ii;

					depth = t;

					closestV0 = v0;
					closestV1 = v1;
					closestV2 = v2;
				}
			}
		}
	}

	if(closestTriangleIndex == -1)
		return float2(0.0f, 0.0f);

	currentNormal = normalize(cross(closestV1 - closestV0, closestV2 - closestV0));

	closestIndex = closestTriangleIndex;

	return float2(u, v);
}

float4 GetTriangleColorAt(int triangleIndex, float2 barycentricCoordinates)
{
	float2 v0 = UnpackTexcoords(vertices[triangles[triangleIndex].indicies.x].texCoord);
	float2 v1 = UnpackTexcoords(vertices[triangles[triangleIndex].indicies.y].texCoord);
	float2 v2 = UnpackTexcoords(vertices[triangles[triangleIndex].indicies.z].texCoord);

	float2 currentTexCoord = v0 + (v1 - v0) * barycentricCoordinates.x + (v2 - v0) * barycentricCoordinates.y;

	currentTexCoord.y = 1.0f - currentTexCoord.y;

	return float4(diffuseTexture.SampleLevel(textureSampler, currentTexCoord.xy, 0).xyz, 0.5f);
}
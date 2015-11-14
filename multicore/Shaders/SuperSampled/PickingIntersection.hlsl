#include "SuperSampledSharedBuffers.h"

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

RWStructuredBuffer<HitData> hitData : register(u0);

cbuffer PickingBuffer : register(b0)
{
	int2 pickingPosition;
};

[numthreads(32, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	uint sphereCount = 0;
	uint triangleCount = 0;
	uint stride = 0;
	
	spheres.GetDimensions(sphereCount, stride);
	triangles.GetDimensions(triangleCount, stride);

	float distance = 0.0f;

	if(threadID.x < sphereCount)
	{
		float3 spherePosition = spheres[threadID.x].position.xyz;
		float sphereRadius = spheres[threadID.x].position.w;

		if(RaySphereIntersection(rayPositions[pickingPosition].xyz, rayDirections[pickingPosition].xyz, spherePosition, sphereRadius, distance))
		{
			hitData[threadID.x].modelIndex = threadID.x;
			hitData[threadID.x].triangleIndex = -1;
			hitData[threadID.x].depth = distance;
		}
		else
		{
			hitData[threadID.x].modelIndex = -1;
			hitData[threadID.x].triangleIndex = -1;
			hitData[threadID.x].depth = -1.0f;
		}
	}
	else if(threadID.x - sphereCount < triangleCount)
	{
		float3 v0 = vertices[triangles[threadID.x - sphereCount].indicies[0]].position.xyz;
		float3 v1 = vertices[triangles[threadID.x - sphereCount].indicies[1]].position.xyz;
		float3 v2 = vertices[triangles[threadID.x - sphereCount].indicies[2]].position.xyz;

		float temp = 0.0f;

		if(RayTriangleIntersection(rayPositions[pickingPosition].xyz, rayDirections[pickingPosition].xyz, v0, v1, v2, temp, temp, distance))
		{
			int modelIndex = -1;

			unsigned int modelCount = 0;
			models.GetDimensions(modelCount, stride);

			for(int i = 0; i < modelCount; ++i)
			{
				int beginIndex = models[i].beginIndex;
				int endIndex = models[i].endIndex;

				if(threadID.x - sphereCount >= beginIndex
					|| threadID.x - sphereCount < endIndex)
				{
					modelIndex = i;
					break;
				}
			}

			hitData[threadID.x].modelIndex = modelIndex;
			hitData[threadID.x].triangleIndex = threadID.x - sphereCount;
			hitData[threadID.x].depth = distance;
		}
		else
		{
			hitData[threadID.x].modelIndex = -1;
			hitData[threadID.x].triangleIndex = -1;
			hitData[threadID.x].depth = -1.0f;
		}
	}
	else
	{
		hitData[threadID.x].modelIndex = -1;
		hitData[threadID.x].triangleIndex = -1;
		hitData[threadID.x].depth = -1.0f;
	}
}
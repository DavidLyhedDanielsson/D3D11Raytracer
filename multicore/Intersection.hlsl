#include "SharedShaderBuffers.h"
//#include "SharedShaderConstants.h"

RWTexture2D<float4> rayPositionsOut : register(u0);
RWTexture2D<float4> rayDirectionsOut : register(u1);
RWTexture2D<float4> rayNormalOut : register(u2);
RWTexture2D<float4> rayColorOut : register(u3);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

Texture2D diffuseTexture : register(t2);

sampler textureSampler : register(s0);

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);
float2 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);

float TripleProduct(float3 a, float3 b, float3 z);

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

		//rayColorOut[threadID.xy] = vertices.color[closestTriangle];
		rayColorOut[threadID.xy] = GetTriangleColorAt(closestTriangle, barycentric);
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, sphereCount + closestTriangle);
	}
	else
	{
		//A sphere was closest

		rayColorOut[threadID.xy] = spheres.color[closestSphere];
		rayPositionsOut[threadID.xy] = float4(rayPosition.xyz + rayDirection * depth, closestSphere);
	}
}

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestSphereIndex = -1;

	for(int i = 0; i < sphereCount; ++i)
	{
		float3 spherePosition = spheres.position[i].xyz;
		float sphereRadius = spheres.position[i].w;

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
	float3 normal = hitPoint - spheres.position[closestSphereIndex].xyz;

	currentNormal = normalize(normal);

	closestIndex = closestSphereIndex;
}

float2 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestTriangleIndex = -1;

	float u = 0.0f;
	float v = 0.0f;

	float3 closestV0 = float3(0.0f, 0.0f, 0.0f);
	float3 closestV1 = float3(0.0f, 0.0f, 0.0f);
	float3 closestV2 = float3(0.0f, 0.0f, 0.0f);

	for(int i = 0; i < triangleCount; ++i)
	{
		float3 v0 = vertices.position[triangles.vertices[i].x].xyz;
		float3 v1 = vertices.position[triangles.vertices[i].y].xyz;
		float3 v2 = vertices.position[triangles.vertices[i].z].xyz;

		float3 e0 = v1 - v0;
		float3 e1 = v2 - v0;

		float3 detCross = cross(rayDirection, e1);
		float det = dot(e0, detCross);

		float detInv = 1.0f / det;

		float3 rayDist = rayPosition - v0;
		float tempU = dot(rayDist, detCross) * detInv;

		if(tempU < 0.0f || tempU > 1.0f)
			continue;

		float3 vPrep = cross(rayDist, e0);
		float tempV = dot(rayDirection, vPrep) * detInv;

		if(tempV < 0.0f || tempU + tempV > 1.0f)
			continue;

		float t = dot(e1, vPrep) * detInv;

		if(t > 0.0f 
			&& t < depth
			&& i + sphereCount != lastHit)
		{
			u = tempU;
			v = tempV;

			closestTriangleIndex = i;

			depth = t;

			closestV0 = v0;
			closestV1 = v1;
			closestV2 = v2;
		}
	}

	if(closestTriangleIndex == -1)
		return float2(0.0f, 0.0f);

	currentNormal = normalize(cross(closestV1 - closestV0, closestV2 - closestV0));

	closestIndex = closestTriangleIndex;

	return float2(u, v);
}

float TripleProduct(float3 a, float3 b, float3 c)
{
	return dot(a, cross(b, c));
}

float4 GetTriangleColorAt(int triangleIndex, float2 barycentricCoordinates)
{
	float2 v0 = vertices.texCoord[triangles.vertices[triangleIndex].x].xy;
	float2 v1 = vertices.texCoord[triangles.vertices[triangleIndex].y].xy;
	float2 v2 = vertices.texCoord[triangles.vertices[triangleIndex].z].xy;

	float2 currentTexCoord = v0 + (v1 - v0) * barycentricCoordinates.x + (v2 - v0) * barycentricCoordinates.y;

	currentTexCoord.y = 1.0f - currentTexCoord.y;

	return float4(diffuseTexture.SampleLevel(textureSampler, currentTexCoord.xy, 0).xyz, 0.5f);
}
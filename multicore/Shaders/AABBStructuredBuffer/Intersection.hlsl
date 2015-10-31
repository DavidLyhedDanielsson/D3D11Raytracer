//#include "AABBStructuredBufferSharedBuffers.h"
#include "AABBStructuredBufferSharedConstants.h"

RWTexture2D<float4> rayPositionsOut : register(u0);
RWTexture2D<float4> rayDirectionsOut : register(u1);
RWTexture2D<float4> rayNormalOut : register(u2);
RWTexture2D<float4> rayColorOut : register(u3);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

Texture2D diffuseTexture : register(t2);

sampler textureSampler : register(s0);

struct Sphere
{
	//Keeping position and color separate plays nice with the cache
	float4 position; //position + radius
	float4 color; //color + reflectivity
};

struct Vertex
{
	//float3 position[MAX_VERTICES]; //position
	//int texCoord[MAX_VERTICES]; //tex coords packed into an int, first 16 bits = u, last 16 bits = v

	float3 position;
	int texCoord;
};

struct Triangle
{
	int4 indicies; //vertices + padding
};

struct AABB
{
	float3 min;
	float3 max;
};

struct Model
{
	AABB aabb;
	int beginIndex;
	int endIndex;
};

StructuredBuffer<Sphere> spheres : register(t4);
StructuredBuffer<Vertex> vertices: register(t5);
StructuredBuffer<Triangle> triangles : register(t6);
StructuredBuffer<Model> models : register(t7);

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);
float2 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);

float TripleProduct(float3 a, float3 b, float3 z);

float4 GetTriangleColorAt(int triangleIndex, float2 barycentricCoordinates);

float2 UnpackTexcoords(int intValue);

bool RayAABBIntersection(float3 rayOrigin, float3 rayDirection, float3 aabbMin, float3 aabbMax);

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
	float3 normal = hitPoint - spheres[closestSphereIndex].position.xyz;

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
					&& ii != lastHit)
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

float TripleProduct(float3 a, float3 b, float3 c)
{
	return dot(a, cross(b, c));
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

float2 UnpackTexcoords(int intValue)
{
	return float2((intValue >> 16) & 0xFFFF, intValue & 0xFFFF) / (float)(0xFFFF);
}
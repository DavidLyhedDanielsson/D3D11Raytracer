//#include "SharedShaderBuffers.h"
#include "SharedShaderConstants.h"

RWTexture2D<float4> rayPositionsOut : register(u0);
RWTexture2D<float4> rayDirectionsOut : register(u1);
RWTexture2D<float4> rayNormalOut : register(u2);
RWTexture2D<float4> rayColorOut : register(u3);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayDirections : register(t1);

sampler textureSampler : register(s0);

struct SphereBufferData
{
	//Keeping position and color separate plays nice with the cache
	float4 position[MAX_SPHERES]; //position + radius
	float4 color[MAX_SPHERES]; //color + reflectivity
};

struct VertexBufferData
{
	float4 position[MAX_VERTICES]; //position + padding
	float4 color[MAX_TRIANGLES]; //color + reflectivity
};

struct TriangleBufferData
{
	int4 vertices[MAX_TRIANGLES]; //vertices + padding
};

struct PointLightBufferData
{
	float4 position[MAX_POINT_LIGHTS]; //position + intensity
};

cbuffer SphereBuffer : register(b0)
{
	SphereBufferData spheres;
	int sphereCount;
};

cbuffer VertexBuffer : register(b1)
{
	VertexBufferData vertices;
};

cbuffer TriangleBuffer : register(b2)
{
	TriangleBufferData triangles;
	int triangleCount;
};

cbuffer pointLightBuffer : register(b3)
{
	PointLightBufferData pointLights;
	int lightCount;
};

void SphereTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);
float3 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex);

float TripleProduct(float3 a, float3 b, float3 z);

float4 GetColorAt(int triangleIndex, float3 barycentricCoordinates);

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
	float3 barycentric = TriangleTrace(rayPosition.xyz, rayDirection, lastHit, depth, normal, closestTriangle);
	
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
		rayColorOut[threadID.xy] = GetColorAt(closestTriangle, barycentric);
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

float3 TriangleTrace(float3 rayPosition, float3 rayDirection, int lastHit, inout float depth, inout float3 currentNormal, inout int closestIndex)
{
	int closestTriangleIndex = -1;

	float u = 0.0f;
	float v = 0.0f;
	float w = 0.0f;

	for(int i = 0; i < triangleCount; ++i)
	{
		/*float3 v0 = vertices.position[triangles.vertices[i].x].xyz;
		float3 v1 = vertices.position[triangles.vertices[i].y].xyz;
		float3 v2 = vertices.position[triangles.vertices[i].z].xyz;

		float3 v0v2 = v2 - v0;
		float3 v0v1 = v1 - v0;
		float3 v0ray = rayPosition - v0;

		float prediv = 1.0f / dot(v0v1, cross(rayDirection, v0v2));
		float3 precross = cross(v0ray, v0v1);

		float distance = dot(v0v2, precross) * prediv;
		u = dot(v0ray, cross(rayDirection, v0v2)) * prediv;
		v = dot(rayDirection, precross) * prediv;

		if(distance > 0.0f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f
			&& distance < depth
			&& sphereCount + i != lastHit)
		{
			closestTriangleIndex = i;
			depth = distance;
		}*/

		if(sphereCount + i == lastHit)
			continue;

		float3 a = vertices.position[triangles.vertices[i].x].xyz;
		float3 b = vertices.position[triangles.vertices[i].y].xyz;
		float3 c = vertices.position[triangles.vertices[i].z].xyz;

		float3 ab = b - a;
		float3 ac = c - a;

		float3 normal = cross(ab, ac);

		float d = dot(-rayDirection, normal);
		if(d <= 0.0f)
			continue;

		float3 ap = rayPosition - a;
		float t = dot(ap, normal);
		if(t < 0.0f)
			continue;

		float3 e = cross(rayDirection, ap);
		float tempV = dot(ab, e);
		if(tempV < 0.0f || tempV > d)
			continue;
		float tempW = -dot(ac, e);
		if(tempW < 0.0f || tempV + tempW > d)
			continue;

		float dInv = 1.0f / d;
		
		t *= dInv;
		if(t > depth)
			continue;

		v = tempV;
		w = tempW;

		v *= dInv;
		w *= dInv;
		u = 1.0f - v - w;

		depth = t;
		closestTriangleIndex = i;
		currentNormal = normalize(normal);
	}

	if(closestTriangleIndex == -1)
		return float3(0.0f, 0.0f, 0.0f);

	//float3 v0 = vertices.position[triangles.vertices[closestTriangleIndex].x].xyz;
	//float3 v1 = vertices.position[triangles.vertices[closestTriangleIndex].y].xyz;
	//float3 v2 = vertices.position[triangles.vertices[closestTriangleIndex].z].xyz;

	//currentNormal = normalize(cross(v2 - v0, v1 - v0));

	closestIndex = closestTriangleIndex;

	return float3(u, v, w);
}

float TripleProduct(float3 a, float3 b, float3 c)
{
	return dot(a, cross(b, c));
}

float4 GetColorAt(int triangleIndex, float3 barycentricCoordinates)
{
	return float4(barycentricCoordinates, 0.0f);

	//if (x + y < 1.0f)
	//{
	//	float4 v0 = vertices.color[triangles.vertices[triangleIndex].x];
	//	float4 v1 = vertices.color[triangles.vertices[triangleIndex].y];
	//	float4 v2 = vertices.color[triangles.vertices[triangleIndex].z];

	//	float v1Area = y;
	//	float v2Area = x;

	//	float v0Area = 1.0f - (v1Area + v2Area);

	//	return float4(v0 * v0Area + v1 * v1Area + v2 * v2Area);
	//}
	//else
	//{
	//	float4 v1 = vertices.color[triangles.vertices[triangleIndex].x];
	//	float4 v2 = vertices.color[triangles.vertices[triangleIndex].y];
	//	float4 v0 = vertices.color[triangles.vertices[triangleIndex].z];

	//	float v1Area = 1.0f - y;
	//	float v2Area = 1.0f - x;
	//	float v0Area = 1.0f - (v1Area + v2Area);

	//	return float4(v0 * v0Area + v1 * v1Area + v2 * v2Area);
	//}
}
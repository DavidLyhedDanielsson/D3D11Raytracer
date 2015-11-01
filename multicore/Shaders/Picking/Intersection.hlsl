//#include "PickingSharedBuffers.h"

bool RayAABBIntersection(float3 rayOrigin, float3 rayDirection, float3 aabbMin, float3 aabbMax)
{
	float3 invDir = 1.0f / rayDirection;

	float t1 = (aabbMin.x - rayOrigin.x) * invDir.x;
	float t2 = (aabbMax.x - rayOrigin.x) * invDir.x;
	float t3 = (aabbMin.y - rayOrigin.y) * invDir.y;
	float t4 = (aabbMax.y - rayOrigin.y) * invDir.y;
	float t5 = (aabbMin.z - rayOrigin.z) * invDir.z;
	float t6 = (aabbMax.z - rayOrigin.z) * invDir.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	if(tmax < 0 || tmin > tmax)
		return false;

	return true;
}

bool RayAABBIntersection(float3 rayPosition, float3 rayDirection, float3 aabbMin, float3 aabbMax, out float t)
{
	float3 invDir = 1.0f / rayDirection;

	float t1 = (aabbMin.x - rayPosition.x) * invDir.x;
	float t2 = (aabbMax.x - rayPosition.x) * invDir.x;
	float t3 = (aabbMin.y - rayPosition.y) * invDir.y;
	float t4 = (aabbMax.y - rayPosition.y) * invDir.y;
	float t5 = (aabbMin.z - rayPosition.z) * invDir.z;
	float t6 = (aabbMax.z - rayPosition.z) * invDir.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	if(tmax < 0 || tmin > tmax)
		return false;

	t = tmin;

	return true;
}

bool RaySphereIntersection(float3 rayPosition, float3 rayDirection, float3 spherePosition, float sphereRadius)
{
	float a = dot(rayDirection, (rayPosition - spherePosition));

	float3 dirToSphere = rayPosition - spherePosition;
	float b = dot(dirToSphere, dirToSphere);

	float root = (a * a) - b + (sphereRadius * sphereRadius);

	return root >= 0.0f;
}

bool RaySphereIntersection(float3 rayPosition, float3 rayDirection, float3 spherePosition, float sphereRadius, out float t)
{
	float a = dot(rayDirection, (rayPosition - spherePosition));

	float3 dirToSphere = rayPosition - spherePosition;
	float b = dot(dirToSphere, dirToSphere);

	float root = (a * a) - b + (sphereRadius * sphereRadius);

	if(root < 0.0f)
		return false;

	t = -a - sqrt(root);

	return true;
}

bool RayTriangleIntersection(float3 rayPosition, float3 rayDirection, float3 v0, float3 v1, float3 v2)
{
	float3 e0 = v1 - v0;
	float3 e1 = v2 - v0;

	float3 detCross = cross(rayDirection, e1);
	float det = dot(e0, detCross);

	float detInv = 1.0f / det;

	float3 rayDist = rayPosition - v0;
	float u = dot(rayDist, detCross) * detInv;

	if(u < 0.0f || u > 1.0f)
		return false;

	float3 vPrep = cross(rayDist, e0);
	float v = dot(rayDirection, vPrep) * detInv;

	if(v < 0.0f || u + v > 1.0f)
		return false;

	return true;
}

bool RayTriangleIntersection(float3 rayPosition, float3 rayDirection, float3 v0, float3 v1, float3 v2, out float outU, out float outV, out float t)
{
	float3 e0 = v1 - v0;
	float3 e1 = v2 - v0;

	float3 detCross = cross(rayDirection, e1);
	float det = dot(e0, detCross);

	float detInv = 1.0f / det;

	float3 rayDist = rayPosition - v0;
	float u = dot(rayDist, detCross) * detInv;

	if(u < 0.0f || u > 1.0f)
		return false;

	float3 vPrep = cross(rayDist, e0);
	float v = dot(rayDirection, vPrep) * detInv;

	if(v < 0.0f || u + v > 1.0f)
		return false;

	t = dot(e1, vPrep) * detInv;

	outU = u;
	outV = v;

	return true;
}

float2 UnpackTexcoords(int intValue)
{
	return float2((intValue >> 16) & 0xFFFF, intValue & 0xFFFF) / (float)(0xFFFF);
}

static const float FLOAT_MAX = 3.4e38f;

static const float SCREEN_RES_X = 1280.0f;
static const float SCREEN_RES_Y = 720.0f;

static const int MAX_POINT_LIGHTS = 10;

const static float AMBIENT_FAC = 0.5f;

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

struct Ray
{
	float3 position;
	float3 direction;
};

struct HitData
{
	int modelIndex;
	int triangleIndex;
	float depth;
	float padding;
};

StructuredBuffer<Sphere> spheres : register(t4);
StructuredBuffer<Vertex> vertices: register(t5);
StructuredBuffer<Triangle> triangles : register(t6);
StructuredBuffer<Model> models : register(t7);


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
	uint stride = 0;
	
	spheres.GetDimensions(sphereCount, stride);

	if(threadID.x >= sphereCount)
		return;

	float3 spherePosition = spheres[threadID.x].position.xyz;
	float sphereRadius = spheres[threadID.x].position.w;

	float distance = 0.0f;

	hitData[threadID.x].modelIndex = -1;
	hitData[threadID.x].triangleIndex = -1;
	hitData[threadID.x].depth = -1.0f;

	if(!RaySphereIntersection(rayPositions[pickingPosition].xyz, rayDirections[pickingPosition].xyz, spherePosition, sphereRadius, distance))
		return;

	hitData[threadID.x].modelIndex = threadID.x;
	hitData[threadID.x].triangleIndex = -1;
	hitData[threadID.x].depth = distance;
}
#ifndef AABBSharedBuffers_h__
#define AABBSharedBuffers_h__

#include "AABBStructuredBufferSharedConstants.h"
#include "../../SharedShaderBuffers.h"

#ifdef _WIN32
//C++
#include <d3d11.h>
#include <dxlib/DXMath.h>

typedef DirectX::XMFLOAT2 float2;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;

typedef DirectX::XMINT2 int2;
typedef DirectX::XMINT3 int3;
typedef DirectX::XMINT4 int4;

#define Buffer(type, identifier, registerID) struct identifier

namespace AABBStructuredBufferSharedBuffers
{
#else
//HLSL
#define Buffer(type, identifier, registerID) StructuredBuffer<type> identifier : register(t##registerID);
#endif // _WIN32

#define SPHERE_BUFFER_REGISTRY_INDEX_DEF 4
#define VERTEX_BUFFER_REGISTRY_INDEX_DEF 5
#define TRIANGLE_BUFFER_REGISTRY_INDEX_DEF 6
#define MODEL_BUFFER_REGISTRY_INDEX_DEF 7

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

#ifdef _WIN32

const static int SPHERE_BUFFER_REGISTRY_INDEX = SPHERE_BUFFER_REGISTRY_INDEX_DEF;
const static int VERTEX_BUFFER_REGISTRY_INDEX = VERTEX_BUFFER_REGISTRY_INDEX_DEF;
const static int TRIANGLE_BUFFER_REGISTRY_INDEX = TRIANGLE_BUFFER_REGISTRY_INDEX_DEF;
const static int MODEL_BUFFER_REGISTRY_INDEX = MODEL_BUFFER_REGISTRY_INDEX_DEF;
}
#else

#define CONCAT(a, b) a ##b

StructuredBuffer<Sphere> spheres : register(CONCAT(t, SPHERE_BUFFER_REGISTRY_INDEX_DEF));
StructuredBuffer<Vertex> vertices: register(CONCAT(t, VERTEX_BUFFER_REGISTRY_INDEX_DEF));
StructuredBuffer<Triangle> triangles : register(CONCAT(t, TRIANGLE_BUFFER_REGISTRY_INDEX_DEF));
StructuredBuffer<Model> models : register(CONCAT(t, MODEL_BUFFER_REGISTRY_INDEX_DEF));

#undef CONCAT
#endif // _WIN32

#undef SPHERE_BUFFER_REGISTRY_INDEX_DEF
#undef VERTEX_BUFFER_REGISTRY_INDEX_DEF
#undef TRIANGLE_BUFFER_REGISTRY_INDEX_DEF
#undef MODEL_BUFFER_REGISTRY_INDEX_DEF
#endif // AABBSharedBuffers_h__
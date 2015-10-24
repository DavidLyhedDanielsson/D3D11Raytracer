#ifndef SharedShaderBuffers_h__
#define SharedShaderBuffers_h__

#include "SharedShaderConstants.h"

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

#define Buffer(identifier, registerID) struct identifier
#else
//HLSL
#define Buffer(identifier, registerID) cbuffer identifier : register(b##registerID)
#endif

#define SPHERE_BUFFER_REGISTRY_INDEX_DEF 0
#define VERTEX_BUFFER_REGISTRY_INDEX_DEF 1
#define TRIANGLE_BUFFER_REGISTRY_INDEX_DEF 2
#define POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF 3

struct SphereBufferData
{
	//Keeping position and color separate plays nice with the cache
	float4 position[MAX_SPHERES]; //position + radius
	float4 color[MAX_SPHERES]; //color + reflectivity
};

struct VertexBufferData
{
	float4 position[MAX_VERTICES]; //position + padding
	float4 texCoord[MAX_TRIANGLES]; //tex coords + reflectivity
};

struct TriangleBufferData
{
	int4 vertices[MAX_TRIANGLES]; //vertices + padding
};

struct PointLightBufferData
{
	float4 position[MAX_POINT_LIGHTS]; //position + intensity
};

Buffer(SphereBuffer, SPHERE_BUFFER_REGISTRY_INDEX_DEF)
{
	SphereBufferData spheres;
	int sphereCount;
};

Buffer(VertexBuffer, VERTEX_BUFFER_REGISTRY_INDEX_DEF)
{
	VertexBufferData vertices;
};

Buffer(TriangleBuffer, TRIANGLE_BUFFER_REGISTRY_INDEX_DEF)
{
	TriangleBufferData triangles;
	int triangleCount;
};

Buffer(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF)
{
	PointLightBufferData pointLights;
	int lightCount;
};

#ifdef _WIN32
const static int SPHERE_BUFFER_REGISTRY_INDEX = SPHERE_BUFFER_REGISTRY_INDEX_DEF;
const static int VERTEX_BUFFER_REGISTRY_INDEX = VERTEX_BUFFER_REGISTRY_INDEX_DEF;
const static int TRIANGLE_BUFFER_REGISTRY_INDEX = TRIANGLE_BUFFER_REGISTRY_INDEX_DEF;
const static int POINT_LIGHT_BUFFER_REGISTRY_INDEX = POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF;

static_assert(sizeof(SphereBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "SphereBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");
static_assert(sizeof(VertexBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "VertexBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");
static_assert(sizeof(TriangleBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "TriangleBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");

#undef Buffer

#undef SPHERE_BUFFER_REGISTRY_INDEX_DEF
#undef VERTEX_BUFFER_REGISTRY_INDEX_DEF
#undef TRIANGLE_BUFFER_REGISTRY_INDEX_DEF
#undef POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF
#endif

#endif // SharedShaderBuffers_h__
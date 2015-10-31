#ifndef ConstantBufferSharedBuffers_h__
#define ConstantBufferSharedBuffers_h__

#include "ConstantBufferSharedConstants.h"
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

#define Buffer(identifier, registerID) struct identifier

namespace ConstantBufferSharedBuffers
{
#else
//HLSL
#define Buffer(identifier, registerID) cbuffer identifier : register(b##registerID)
#endif

#define SPHERE_BUFFER_REGISTRY_INDEX_DEF 1
#define VERTEX_BUFFER_REGISTRY_INDEX_DEF 2
#define TRIANGLE_BUFFER_REGISTRY_INDEX_DEF 3

struct SphereBufferData
{
	//Keeping position and color separate plays nice with the cache
	float4 position[MAX_SPHERES]; //position + radius
	float4 color[MAX_SPHERES]; //color + reflectivity
};

struct VertexBufferData
{
	//float3 position[MAX_VERTICES]; //position
	//int texCoord[MAX_VERTICES]; //tex coords packed into an int, first 16 bits = u, last 16 bits = v

	float3 position;
	int texCoord;
};

struct TriangleBufferData
{
	int4 indicies[MAX_TRIANGLES]; //vertices + padding
};

Buffer(SphereBuffer, SPHERE_BUFFER_REGISTRY_INDEX_DEF)
{
	SphereBufferData spheres;
	int sphereCount;
};

Buffer(VertexBuffer, VERTEX_BUFFER_REGISTRY_INDEX_DEF)
{
	VertexBufferData vertices[MAX_VERTICES];
};

Buffer(TriangleBuffer, TRIANGLE_BUFFER_REGISTRY_INDEX_DEF)
{
	TriangleBufferData triangles;
	int triangleCount;
};

#ifdef _WIN32
const static int SPHERE_BUFFER_REGISTRY_INDEX = SPHERE_BUFFER_REGISTRY_INDEX_DEF;
const static int VERTEX_BUFFER_REGISTRY_INDEX = VERTEX_BUFFER_REGISTRY_INDEX_DEF;
const static int TRIANGLE_BUFFER_REGISTRY_INDEX = TRIANGLE_BUFFER_REGISTRY_INDEX_DEF;
}

static_assert(sizeof(ConstantBufferSharedBuffers::SphereBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "SphereBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");
static_assert(sizeof(ConstantBufferSharedBuffers::VertexBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "VertexBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");
static_assert(sizeof(ConstantBufferSharedBuffers::TriangleBuffer) / 16.0f <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, "TriangleBuffer has more than D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT elements");

#endif

#undef Buffer
#undef SPHERE_BUFFER_REGISTRY_INDEX_DEF
#undef VERTEX_BUFFER_REGISTRY_INDEX_DEF
#undef TRIANGLE_BUFFER_REGISTRY_INDEX_DEF

#endif // ConstantBufferSharedBuffers_h__
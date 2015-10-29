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

#define POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF 0

struct PointLightBufferData
{
	float4 position[MAX_POINT_LIGHTS]; //position + intensity
};

Buffer(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF)
{
	PointLightBufferData pointLights;
	int lightCount;
};

#ifdef _WIN32
const static int POINT_LIGHT_BUFFER_REGISTRY_INDEX = POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF;
#endif

#undef Buffer
#undef POINT_LIGHT_BUFFER_REGISTRY_INDEX_DEF

#endif // SharedShaderBuffers_h__
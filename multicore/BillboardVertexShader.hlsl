#include "Constants.hlsl"

cbuffer cameraData : register(b0)
{
	float4x4 viewProjMatrix;
	float3 cameraPosition;
};

cbuffer cbPerObject : register(b1)
{
	float4x4 worldMatrix[MAX_LIGHTS];
};

struct VSIn
{
	float3 position : POSITION;
	float2 texCoord : TEX_COORD;
	uint instanceID : SV_InstanceID;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

VSOut main(VSIn inData)
{
	VSOut outData;

	outData.position = mul(mul(worldMatrix[inData.instanceID], float4(inData.position, 1.0f)), viewProjMatrix);
	outData.texCoord = inData.texCoord;

	return outData;
}
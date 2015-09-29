#include "Constants.hlsl"

RWTexture2D<float4> outputPosition : register(u0);
RWTexture2D<float4> outputDirection : register(u1);
RWTexture2D<float4> outputNormal : register(u2);

cbuffer viewProjBuffer : register(b0)
{
	float4x4 viewProjMatrixInv;
};

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	//Convert threadID to NDC coords
	float2 ndcCoords = threadID.xy / float2(1280.0f, 720.0f) * 2.0f;

	ndcCoords.x -= 1.0f;
	ndcCoords.y = 1.0f - ndcCoords.y;

	float3 minNDC = float3(ndcCoords, 0.0f);
	float3 maxNDC = float3(ndcCoords, 1.0f);

	//Use NDC coords to calculate direction vector
	float4 maxWorld = mul(float4(maxNDC, 1.0f), viewProjMatrixInv);
	maxWorld /= maxWorld.w;

	//Origin of ray
	float4 origin = mul(float4(minNDC, 1.0f), viewProjMatrixInv);
	origin /= origin.w;

	outputPosition[threadID.xy] = float4(origin.xyz, FLOAT_MAX); //Use fourth channel as depth buffer
	outputDirection[threadID.xy] = float4(normalize(maxWorld.xyz - origin.xyz), 1.0f);
	outputNormal[threadID.xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);
}
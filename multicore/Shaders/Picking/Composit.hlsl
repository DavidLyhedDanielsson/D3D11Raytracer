#include "SuperSampledSharedConstants.h"

RWTexture2D<float4> backBuffer : register(u0);
RWTexture2D<float> depthBuffer : register(u1);

Texture2D<float4> colors : register(t0);
Texture2D<float> depth : register(t1);

cbuffer superSampleBuffer : register(b0)
{
	uint superSampleCount;
};

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float3 accumulatedColor = float3(0.0f, 0.0f, 0.0f);
	float accumulatedAlpha = 0.0f;
	float outDepth = -FLOAT_MAX;

	for(uint y = 0; y < superSampleCount; ++y)
	{
		for(uint x = 0; x < superSampleCount; ++x)
		{
			accumulatedColor += colors[uint2(threadID.x * superSampleCount + x, threadID.y * superSampleCount + y)].xyz;
			float currentDepth = depth[uint2(threadID.x * superSampleCount + x, threadID.y * superSampleCount + y)];

			outDepth = max(outDepth, currentDepth);
			accumulatedAlpha += currentDepth < FLOAT_MAX;
		}
	}

	float3 outColor = accumulatedColor / (superSampleCount * superSampleCount);
	float outAlpha = accumulatedAlpha / (superSampleCount * superSampleCount);

	backBuffer[threadID.xy] = float4(outColor, outAlpha);
	depthBuffer[threadID.xy] = outDepth;
}
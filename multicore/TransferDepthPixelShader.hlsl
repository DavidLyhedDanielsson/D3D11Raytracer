Texture2D<float> depthTexture : register(t0);

sampler depthSamplerState : register(s0);

struct VertexOut
{
	float4 svPosition : SV_POSITION;
	float2 position : POSITION;
};

struct PixelOut
{
	//float4 target : SV_TARGET;
	float depth : SV_DEPTH;
};

cbuffer projBuffer : register(b0)
{
	float4x4 projMatrix;
};

PixelOut main(VertexOut inData)
{
	PixelOut outData;

	float2 texCoords = float2((inData.position.x + 1.0f) * 0.5f, 1.0f - ((inData.position.y + 1.0f) * 0.5f));

	float depth = depthTexture.Sample(depthSamplerState, texCoords);

	float4 projectedDepth = mul(float4(0.0f, 0.0f, depth, 1.0f), projMatrix);
	outData.depth = projectedDepth.z / projectedDepth.w;

	//outData.target.x = projectedDepth.z / projectedDepth.w;
	//outData.target.y = projectedDepth.z / projectedDepth.w;
	//outData.target.z = projectedDepth.z / projectedDepth.w;
	//outData.target.w = depth;

	//outData.target = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return outData;
}
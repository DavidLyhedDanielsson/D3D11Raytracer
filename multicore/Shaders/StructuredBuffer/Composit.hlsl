RWTexture2D<float4> backbuffer : register(u0);

Texture2D<float4> colors : register(t0);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float3 outColor = colors[threadID.xy].xyz;

	backbuffer[threadID.xy] = float4(outColor, 1.0f);
}
RWTexture2D<float4> backbuffer : register(u0);

Texture2D<float4> colors : register(t0);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	backbuffer[threadID.xy] = float4(colors[threadID.xy].xyz, 1.0f);
}
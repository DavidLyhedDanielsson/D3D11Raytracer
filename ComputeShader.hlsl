RWTexture2D<float4> output : register(u0);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	output[threadID.xy] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}
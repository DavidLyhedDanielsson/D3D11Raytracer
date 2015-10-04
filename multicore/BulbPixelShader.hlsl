struct VSOut
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

Texture2D bulbTexture : register(t0);
SamplerState bulbSampler : register(s0);

float4 main(VSOut inData) : SV_TARGET
{
	return bulbTexture.Sample(bulbSampler, inData.texCoord);
}
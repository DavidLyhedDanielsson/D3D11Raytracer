struct VSOut
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORDS;
	float4 color : COLOR;
};

Texture2D currentTexture : register(t0);
SamplerState currentSampler : register(s0);

float4 main(VSOut inData) : SV_TARGET
{
	return inData.color;
}

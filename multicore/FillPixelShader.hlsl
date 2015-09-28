struct VSOut
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORDS;
	float4 color : COLOR;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(VSOut inData) : SV_TARGET
{
	float4 textureColor = tex.Sample(samp, inData.texCoords);

	return textureColor * inData.color;
}

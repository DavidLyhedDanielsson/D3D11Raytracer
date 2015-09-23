struct VSIn
{
	float2 position : POSITION;
	float2 texCoords : TEXCOORDS;
	float4 color : COLOR;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORDS;
	float4 color : COLOR;
};

cbuffer viewProjBuffer : register(b0)
{
	matrix viewProjMatrix;
};

VSOut main(VSIn inData)
{
	VSOut outData;

	outData.position = mul(float4(inData.position, 0.5f, 1.0f), viewProjMatrix);
	outData.texCoords = inData.texCoords;
	outData.color = inData.color;

	return outData;
}

struct VSIn
{
	float3 position : POSITION;
	float3 color : COLOR;
	float2 padding : PADDING;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

VSOut main(VSIn input)
{
	VSOut output;

	output.position = float4(input.position, 1.0f);
	output.color = input.color;

	return output;
}
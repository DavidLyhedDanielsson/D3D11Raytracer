struct VSOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float dashValue : DASH_VALUE;
};

float4 main(VSOut inData) : SV_TARGET
{
	float outColor = sin(inData.dashValue);
	float stepValue = 1.0f - step(outColor, 0);

	return float4(inData.color.xyz, stepValue);
}
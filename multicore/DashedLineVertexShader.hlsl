#include "Constants.hlsl"

struct VSIn
{
	float2 position : POSITION;
	float4 color : COLOR;
	float dashValue : DASH_VALUE;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float dashValue : DASH_VALUE;
};

VSOut main(VSIn inData)
{
	VSOut outData;

	float2 outPosition = (inData.position / float2(SCREEN_RES_X, SCREEN_RES_Y)) * 2.0f;
	outPosition.x -= 1.0f;
	outPosition.y = 1.0f - outPosition.y;

	outData.position = float4(outPosition, 0.5f, 1.0f);

	outData.color = inData.color;
	outData.dashValue = inData.dashValue;

	return outData;
}
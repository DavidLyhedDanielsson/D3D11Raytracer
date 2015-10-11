#include "BezierConstants.hlsl"

cbuffer cameraData : register(b0)
{
	float4x4 viewProjMatrix;
};

float3 CubicBezier(float lambda, float3 beginAnchor, float3 endAnchor, float3 beginHandle, float3 endHandle);

[domain("isoline")]
DSOut main(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<BezierNode, 2> inData)
{
	DSOut outData;

	//float3 dir = isoline[1].position - isoline[0].position;
	//outData.position = mul(float4(isoline[0].position + dir * uv.x, 1.0f), viewProjMatrix);

	float percent = 1.0f / patchTess.edgeTess[0];
	outData.position = mul(float4(CubicBezier(uv.y + uv.x * percent, inData[0].beginAnchor, inData[0].endAnchor, inData[0].beginHandle, inData[0].endHandle), 1.0f), viewProjMatrix);
	
	return outData;
}

float3 CubicBezier(float lambda, float3 beginAnchor, float3 endAnchor, float3 beginHandle, float3 endHandle)
{
	float3 cubicAddend = (lambda * lambda * lambda) * (-beginAnchor + 3.0f * beginHandle - 3.0f * endHandle + endAnchor);
	float3 quadraticAddend = (lambda * lambda) * (3.0f * beginAnchor - 6.0f * beginHandle + 3.0f * endHandle);
	float3 linearAddend = lambda * (-3.0f * beginAnchor + 3.0f * beginHandle);

	return cubicAddend + quadraticAddend + linearAddend + beginAnchor;
}
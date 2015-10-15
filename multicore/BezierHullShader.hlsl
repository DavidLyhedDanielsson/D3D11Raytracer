#include "BezierConstants.hlsl"

cbuffer TessFactors : register(b0)
{
	float firstTessFactor;
	float secondTessFactor;
};

PatchTess ConstantMain(InputPatch<BezierNode, 2> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess outData;

	outData.edgeTess[0] = firstTessFactor;
	outData.edgeTess[1] = secondTessFactor;

	return outData;
}

[domain("isoline")]
[partitioning("integer")]
[outputtopology("line")]
[outputcontrolpoints(2)]
[patchconstantfunc("ConstantMain")]
[maxtessfactor(64.0f)]
BezierNode main(InputPatch<BezierNode, 2> p, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
	return p[i];
}
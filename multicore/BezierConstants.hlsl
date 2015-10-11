struct BezierNode
{
	float3 beginAnchor : BEGIN_ANCHOR;
	float3 endAnchor : END_ANCHOR;
	float3 beginHandle : BEGIN_HANDLE;
	float3 endHandle : END_HANDLE;
	float lambda : LAMBDA;
};

struct PatchTess
{
	float edgeTess[2] : SV_TessFactor;
};

struct DSOut
{
	float4 position : SV_POSITION;
};
struct VertexOut
{
	float4 svPosition : SV_POSITION;
	float2 position : POSITION;
};

VertexOut main(float2 pos : POSITION)
{
	VertexOut vertexOut;

	vertexOut.svPosition = float4(pos, 0.5f, 1.0f);
	vertexOut.position = pos;

	return vertexOut;
}
#include "Constants.hlsl"

RWTexture2D<float4> output : register(u0);
RWTexture2D<float4> rayPositions : register(u1);
RWTexture2D<float4> rayDirections : register(u2);

cbuffer sphereBuffer : register(b0)
{
	float4 vertices[MAX_TRIANGLES * 3];
	int triangleCount;
};

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float3 rayPosition = rayPositions[threadID.xy].xyz;
	float3 u = rayDirections[threadID.xy].xyz;

	int closestTriangleIndex = -1;
	float closestTriangleDistance = rayPositions[threadID.xy].w;

	for(int i = 0; i < triangleCount; ++i)
	{
		float3 v0 = vertices[i * 3].xyz;
		float3 v1 = vertices[i * 3 + 1].xyz;
		float3 v2 = vertices[i * 3 + 2].xyz;

		float3 e2 = v2 - v0;
		float3 e1 = v1 - v0;
		float3 m = rayPosition - v0;

		float prediv = 1.0f / dot(e1, cross(u, e2));
		float3 precross = cross(m, e1);

		float distance = dot(e2, precross) * prediv;
		float a = dot(m, cross(u, e2)) * prediv;
		float b = dot(u, precross) * prediv;

		if(distance >= 0.0f && a >= 0.0f && b >= 0.0f && a + b <= 1.0f
			&& distance < closestTriangleDistance)
		{
			closestTriangleIndex = i;
			closestTriangleDistance = distance;
		}
	}

	if(closestTriangleIndex == -1)
		return;

	float3 v0 = vertices[closestTriangleIndex * 3].xyz;
	float3 v1 = vertices[closestTriangleIndex * 3 + 1].xyz;
	float3 v2 = vertices[closestTriangleIndex * 3 + 2].xyz;

	float3 normal = cross(v2 - v0, v1 - v0);

	float lightFac = dot(normalize(normal), LIGHT_DIR);

	output[threadID.xy] = float4(lightFac, lightFac, lightFac, 1.0f);
	rayPositions[threadID.xy] = float4(rayPosition, closestTriangleDistance);
}
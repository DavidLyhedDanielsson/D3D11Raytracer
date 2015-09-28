#include "Constants.hlsl"

RWTexture2D<float4> output : register(u0);
RWTexture2D<float4> rayPositions : register(u1);
RWTexture2D<float4> rayDirections : register(u2);

cbuffer sphereBuffer : register(b0)
{
	float4 spheres[MAX_SPHERES];
	int sphereCount;
};

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float2 texCoords = threadID.xy / float2(1280.0f, 720.0f);

	float3 rayPosition = rayPositions[threadID.xy].xyz;
	float3 rayDirection = rayDirections[threadID.xy].xyz;

	int closestSphereIndex = -1;
	float closestSphereDistance = rayPositions[threadID.xy].w;

	for(int i = 0; i < sphereCount; ++i)
	{
		float3 spherePosition = spheres[i].xyz;
		float sphereRadius = spheres[i].w;

		float a = dot(rayDirection, (rayPosition - spherePosition));

		float3 dirToSphere = rayPosition - spherePosition;
		float b = dot(dirToSphere, dirToSphere);

		float root = (a * a) - b + (sphereRadius * sphereRadius);

		if(root < 0.0f)
			continue;

		float distance = -a - sqrt(root);

		if(distance < closestSphereDistance
			&& distance >= 0.0f)
		{
			closestSphereDistance = distance;
			closestSphereIndex = i;
		}
	}

	if(closestSphereIndex == -1)
		return;

	float3 closestSphere = spheres[closestSphereIndex].xyz;

	float3 hitPoint = rayPosition + rayDirection * closestSphereDistance;
	float3 normal = hitPoint - closestSphere;

	float lightFac = dot(normalize(normal), LIGHT_DIR);

	output[threadID.xy] = float4(lightFac, lightFac, lightFac, 1.0f);
	rayPositions[threadID.xy] = float4(rayPosition, closestSphereDistance);
}
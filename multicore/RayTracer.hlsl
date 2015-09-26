RWTexture2D<float4> output : register(u0);

Texture2D rayPositions : register(t0);
Texture2D rayDirections : register(t1);

SamplerState raySampler : register(s0);

cbuffer sphereBuffer : register(b0)
{
	float4 spheres[64];
};

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float2 texCoords = threadID.xy / float2(1280.0f, 720.0f);

	float3 rayPosition = rayPositions.SampleLevel(raySampler, texCoords, 0).xyz;
	float3 rayDirection = rayDirections.SampleLevel(raySampler, texCoords, 0).xyz;

	int closestSphereIndex = -1;
	float closestSphereDistance = 2e30f;

	for(int i = 0; i < 64; ++i)
	{
		float3 spherePosition = spheres[i].xyz;
		float sphereRadius = spheres[i].w;

		float a = dot(rayDirection, (rayPosition - spherePosition));

		float3 dirToSphere = rayPosition - spherePosition;
		float b = dot(dirToSphere, dirToSphere);

		float root = (a * a) - b + (sphereRadius * sphereRadius);

		float distance = -a - sqrt(root);

		if(distance >= 0.0f && distance < closestSphereDistance)
		{
			closestSphereDistance = distance;
			closestSphereIndex = i;
		}
	}

	if(closestSphereIndex == -1)
	{
		output[threadID.xy] = float4(44.0f, 87.0f, 120.0f, 255.0f) / 255.0f;
		return;
	}

	const float3 LIGHT_DIR = float3(0.5f, 0.5f, 0.0f);

	float3 closestSphere = spheres[closestSphereIndex].xyz;

	float3 hitPoint = rayPosition + rayDirection * closestSphereDistance;
	float3 normal = hitPoint - closestSphere;

	float lightFac = dot(normalize(normal), normalize(LIGHT_DIR));

	if(closestSphereDistance < 1e30f)
		output[threadID.xy] = float4(1.0f, 1.0f, 1.0f, 1.0f) * lightFac;
	else
		output[threadID.xy] = float4(44.0f, 87.0f, 120.0f, 255.0f) / 255.0f;
}
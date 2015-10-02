#include "Constants.hlsl"

cbuffer pointlightBuffer : register(b0)
{
	float4 lights[MAX_LIGHTS];
	int lightCount;
};

cbuffer sphereBuffer : register(b1)
{
	float4 spheres[MAX_SPHERES];
	int sphereCount;
};

cbuffer triangleBuffer : register(b2)
{
	float4 vertices[MAX_TRIANGLES * 3];
	int triangleCount;
};

RWTexture2D<float4> output : register(u0);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayNormals : register(t1);

sampler textureSampler : register(s0);

bool SphereTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection);
bool TriangleTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection);

const static float AMBIENT_FAC = 0.25f;

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float3 normal = rayNormals[threadID.xy].xyz;
	float lightFac = AMBIENT_FAC;

	if(dot(normal, normal) != 0.0f)
	{
		float4 rayPositionAndDepth = rayPositions[threadID.xy];

		for(int i = 0; i < lightCount; i++)
		{
			float3 rayLight = lights[i].xyz - rayPositionAndDepth.xyz;
			float distanceToLight = length(rayLight);
			float3 rayDirection = normalize(rayLight);

			if(distanceToLight > lights[i].w)
				continue;

			if(SphereTrace(rayPositionAndDepth.xyz, lights[i].xyz, distanceToLight, rayDirection) && TriangleTrace(rayPositionAndDepth.xyz, lights[i].xyz, distanceToLight, rayDirection))
			{
				float attenuation = 1.0f - min((distanceToLight / lights[i].w), 1.0f);

				float3 rayLightDir = normalize(lights[i].xyz - rayPositionAndDepth.xyz);
			
				//Diffuse lighting
				lightFac += max(0.0f, dot(rayLightDir, normal)) * attenuation;
			}
		}
	}

	output[threadID.xy] = float4(lightFac, lightFac, lightFac, 1.0f);
}


bool SphereTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection)
{
	int closestSphereIndex = -1;

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

		float distance0 = -a + sqrt(root);

		if(distance0 >= 0.5f && distance0 < distanceToLight * 0.95f)
			return false;
	}

	return true;
}

bool TriangleTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection)
{
	int closestTriangleIndex = -1;

	for(int i = 0; i < triangleCount; ++i)
	{
		float3 v0 = vertices[i * 3].xyz;
		float3 v1 = vertices[i * 3 + 1].xyz;
		float3 v2 = vertices[i * 3 + 2].xyz;

		float3 v0v2 = v2 - v0;
		float3 v0v1 = v1 - v0;
		float3 v0ray = rayPosition - v0;

		float prediv = 1.0f / dot(v0v1, cross(rayDirection, v0v2));
		float3 precross = cross(v0ray, v0v1);

		float distance = dot(v0v2, precross) * prediv;
		float x = dot(v0ray, cross(rayDirection, v0v2)) * prediv;
		float y = dot(rayDirection, precross) * prediv;

		if(distance >= 0.5f && x >= 0.0f && y >= 0.0f && x + y <= 1.0f
			&& distance < distanceToLight * 0.5f)
		{
			return false;
		}
	}

	return true;
}
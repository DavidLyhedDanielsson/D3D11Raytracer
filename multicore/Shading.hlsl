#include "Constants.hlsl"

cbuffer pointlightBuffer : register(b0)
{
	float4 lights[MAX_LIGHTS];
	int lightCount;
};

cbuffer sphereBuffer : register(b1)
{
	float4 spheres[MAX_SPHERES];
	float4 sphereColors[MAX_SPHERES];
	int sphereCount;
};

cbuffer triangleBuffer : register(b2)
{
	float4 vertices[MAX_TRIANGLES * 3];
	int triangleCount;
};

cbuffer attenuationBuffer : register(b3)
{
	float a;
	float b;
	float c;
};

cbuffer cameraPositionBuffer : register(b4)
{
	float3 cameraPosition;
}

RWTexture2D<float4> backbufferOut : register(u0);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayNormals : register(t1);
Texture2D<float4> rayColors : register(t2);
Texture2D<float4> backbufferIn : register(t3);

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
		float3 rayPosition = rayPositions[threadID.xy].xyz;
		
		for(int i = 0; i < lightCount; i++)
		{
			float3 rayLight = lights[i].xyz - rayPosition;
			float distanceToLight = length(rayLight);
			float3 rayDirection = normalize(rayLight);

			if(SphereTrace(rayPosition, lights[i].xyz, distanceToLight, rayDirection) && TriangleTrace(rayPosition, lights[i].xyz, distanceToLight, rayDirection))
			{
				float3 rayLightDir = normalize(lights[i].xyz - rayPosition);
			
				//Diffuse lighting
				float diffuseFac = max(0.0f, dot(rayLightDir, normal));

				float specularFac = 0.0f;

				if(diffuseFac > 0.0f)
				{
					//Specular lighting
					float3 rayCamera = normalize(cameraPosition - rayPosition);

					float3 reflection = reflect(-rayDirection, normal);
					specularFac = pow(max(dot(reflection, rayCamera), 0.0f), 32.0f);
				}

				lightFac += (lights[i].w * diffuseFac + specularFac) / (a * (distanceToLight * distanceToLight) + b * distanceToLight + c);
			}
		}
	}

	lightFac = saturate(lightFac);

	float3 oldColor = backbufferIn[threadID.xy].xyz * 0.5f;
	float3 color = rayColors[threadID.xy].xyz * 1.0;

	backbufferOut[threadID.xy] = float4(oldColor + color * lightFac, 1.0f);
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
			&& distance < distanceToLight * 0.95f)
		{
			return false;
		}
	}

	return true;
}
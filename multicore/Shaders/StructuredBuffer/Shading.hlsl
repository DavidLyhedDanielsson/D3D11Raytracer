#include "StructuredBufferSharedBuffers.h"

cbuffer attenuationBuffer : register(b4)
{
	float a;
	float b;
	float c;
};

cbuffer cameraPositionBuffer : register(b5)
{
	float3 cameraPosition;
}

RWTexture2D<float4> backbufferOut : register(u0);

Texture2D<float4> rayPositions : register(t0);
Texture2D<float4> rayNormals : register(t1);
Texture2D<float4> rayColors : register(t2);
Texture2D<float4> backbufferIn : register(t3);

sampler textureSampler : register(s0);

bool SphereTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection, int lastHit);
bool TriangleTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection, int lastHit);

[numthreads(32, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	float3 normal = rayNormals[threadID.xy].xyz;
	float lightFac = AMBIENT_FAC;
	float specularFac = 0.0f;

	if(dot(normal, normal) != 0.0f
		&& backbufferIn[threadID.xy].w > 0.01f)
	{
		float3 rayPosition = rayPositions[threadID.xy].xyz;
		int lastHit = rayPositions[threadID.xy].w;
		
		for(int i = 0; i < lightCount; i++)
		{
			float3 rayLight = pointLights.position[i].xyz - rayPosition;
			float distanceToLight = length(rayLight);
			float3 rayDirection = normalize(rayLight);

			if(SphereTrace(rayPosition, pointLights.position[i].xyz, distanceToLight, rayDirection, lastHit) && TriangleTrace(rayPosition, pointLights.position[i].xyz, distanceToLight, rayDirection, lastHit))
			{
				float3 rayLightDir = normalize(pointLights.position[i].xyz - rayPosition);
			
				//Diffuse lighting
				float diffuseFac = max(0.0f, dot(rayLightDir, normal));

				float attenuation = 1.0f / (a * (distanceToLight * distanceToLight) + b * distanceToLight + c);

				if(diffuseFac > 0.0f)
				{
					//Specular lighting
					float3 rayCamera = normalize(cameraPosition - rayPosition);

					float3 reflection = reflect(-rayDirection, normal);
					specularFac += pow(max(dot(reflection, rayCamera), 0.0f), 32.0f) * 4.0f * attenuation;
				}


				lightFac += (pointLights.position[i].w * diffuseFac) * attenuation;
			}
		}

		lightFac = saturate(lightFac);
		specularFac = saturate(specularFac);

		float3 oldColor = backbufferIn[threadID.xy].xyz;
		float3 color = rayColors[threadID.xy].xyz * backbufferIn[threadID.xy].w * (1.0f - rayColors[threadID.xy].w);

		float3 outColor = oldColor + color * lightFac + specularFac;

		backbufferOut[threadID.xy] = float4(outColor, backbufferIn[threadID.xy].w * rayColors[threadID.xy].w);
	}
	else
	{
		backbufferOut[threadID.xy] = float4(backbufferIn[threadID.xy].xyz, 0.0f);
	}
}

bool SphereTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection, int lastHit)
{
	int closestSphereIndex = -1;

	uint sphereCount = 0;
	uint stride = 0;

	spheres.GetDimensions(sphereCount, stride);

	for(int i = 0; i < (int)sphereCount; ++i)
	{
		float3 spherePosition = spheres[i].position.xyz;
		float sphereRadius = spheres[i].position.w;

		float a = dot(rayDirection, (rayPosition - spherePosition));

		float3 dirToSphere = rayPosition - spherePosition;
		float b = dot(dirToSphere, dirToSphere);

		float root = (a * a) - b + (sphereRadius * sphereRadius);

		if(root < 0.0f)
			continue;

		float distance0 = -a + sqrt(root);

		if(distance0 > 0.0f && distance0 < distanceToLight && i != lastHit)
			return false;
	}

	return true;
}

bool TriangleTrace(float3 rayPosition, float3 lightPosition, float distanceToLight, float3 rayDirection, int lastHit)
{
	int closestTriangleIndex = -1;

	uint sphereCount = 0;
	uint stride = 0;

	spheres.GetDimensions(sphereCount, stride);

	uint triangleCount = 0;

	triangles.GetDimensions(triangleCount, stride);

	for(int i = 0; i < (int)triangleCount; ++i)
	{
		float3 v0 = vertices[triangles[i].indicies.x].position.xyz;
		float3 v1 = vertices[triangles[i].indicies.y].position.xyz;
		float3 v2 = vertices[triangles[i].indicies.z].position.xyz;

		float3 e0 = v1 - v0;
		float3 e1 = v2 - v0;

		float3 detCross = cross(rayDirection, e1);
		float det = dot(e0, detCross);

		float detInv = 1.0f / det;

		float3 rayDist = rayPosition - v0;
		float tempU = dot(rayDist, detCross) * detInv;

		if(tempU < 0.0f || tempU > 1.0f)
			continue;

		float3 vPrep = cross(rayDist, e0);
		float tempV = dot(rayDirection, vPrep) * detInv;

		if(tempV < 0.0f || tempU + tempV > 1.0f)
			continue;

		float t = dot(e1, vPrep) * detInv;

		if(t > 0.0f 
			&& (int)sphereCount + i != lastHit
			&& t < distanceToLight * 0.95f)
		{
			return false;
		}


		//float3 v0 = vertices.position[triangles.vertices[i].x].xyz;
		//float3 v1 = vertices.position[triangles.vertices[i].y].xyz;
		//float3 v2 = vertices.position[triangles.vertices[i].z].xyz;

		//float3 v0v2 = v2 - v0;
		//float3 v0v1 = v1 - v0;
		//float3 v0ray = rayPosition - v0;

		//float prediv = 1.0f / dot(v0v1, cross(rayDirection, v0v2));
		//float3 precross = cross(v0ray, v0v1);

		//float distance = dot(v0v2, precross) * prediv;
		//float x = dot(v0ray, cross(rayDirection, v0v2)) * prediv;
		//float y = dot(rayDirection, precross) * prediv;

		//if(distance > 0.0f && x >= 0.0f && y >= 0.0f && x + y <= 1.0f
		//	&& distance < distanceToLight
		//	&& sphereCount + i != lastHit)
		//{
		//	return false;
		//}



		//if(sphereCount + i == lastHit)
		//	continue;

		//float3 a = vertices.position[triangles.vertices[i].x].xyz;
		//float3 b = vertices.position[triangles.vertices[i].y].xyz;
		//float3 c = vertices.position[triangles.vertices[i].z].xyz;

		//float3 ab = b - a;
		//float3 ac = c - a;

		//float3 normal = cross(ac, ab);

		//float d = dot(-rayDirection, normal);
		//if(d < 0.0f)
		//	continue;

		//float3 ap = rayPosition - a;
		//float t = dot(ap, normal);
		//if(t < 0.0f)
		//	continue;

		//float3 e = cross(rayDirection, ap);
		//float tempV = dot(ac, e);
		//if(tempV < 0.0f || tempV > d)
		//	continue;
		//float tempW = -dot(ab, e);
		//if(tempW < 0.0f || tempV + tempW > d)
		//	continue;

		//float dInv = 1.0f / d;
		
		//t *= dInv;
		//if(t < distanceToLight)
		//	return false;
	}

	return true;
}
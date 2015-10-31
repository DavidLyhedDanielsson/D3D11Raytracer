#include "ConstantBufferShaderProgram.h"

#include <DXLib/ShaderResourceBinds.h>
#include <DXLib/States.h>
#include <DXLib/OBJFile.h>

ConstantBufferShaderProgram::ConstantBufferShaderProgram()
	: primaryRayGenerator("main", "cs_5_0")
	, traceShader("main", "cs_5_0")
	, intersectionShader("main", "cs_5_0")
	, compositShader("main", "cs_5_0")
{
}

bool ConstantBufferShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	if(!ShaderProgram::Init(device, deviceContext, backBufferWidth, backBufferHeight, console, contentManager))
		return false;

	dispatchX = backBufferWidth / 32;
	dispatchY = backBufferHeight / 16;

	sphereBufferData.sphereCount = 0;
	triangleBufferData.triangleCount = 0;

	shaderPath = "Shaders/ConstantBuffer/";

	return true;
}

bool ConstantBufferShaderProgram::InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV)
{
	if(!ShaderProgram::InitBuffers(depthBufferUAV, backBufferUAV))
		return false;

	LogErrorReturnFalse(sphereBuffer.Create<ConstantBufferSharedBuffers::SphereBuffer>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &sphereBufferData), "Couldn't create sphere buffer: ");
	LogErrorReturnFalse(triangleVertexBuffer.Create<ConstantBufferSharedBuffers::VertexBuffer>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &vertexBufferData), "Couldn't create triangle vertex buffer: ");
	LogErrorReturnFalse(triangleBuffer.Create<ConstantBufferSharedBuffers::TriangleBuffer>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &triangleBufferData), "Couldn't create triangle index buffer: ");
	LogErrorReturnFalse(viewProjInverseBuffer.Create<DirectX::XMFLOAT4X4>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create triangle index buffer: ");

	if(!InitUAVSRV())
		return false;
	if(!InitShaders())
		return false;

	return true;
}

bool ConstantBufferShaderProgram::InitUAVSRV()
{
	//Position
	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayPositionUAV[0], rayPositionSRV[0]), "Couldn't create ray position combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayPositionUAV[1], rayPositionSRV[1]), "Couldn't create ray position combo");

	//Direction
	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayDirectionUAV[0], rayDirectionSRV[0]), "Couldn't create ray direction combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayDirectionUAV[1], rayDirectionSRV[1]), "Couldn't create ray direction combo");

	//Normal
	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayNormalUAV, rayNormalSRV), "Couldn't create ray normal combo");

	//Color
	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, rayColorUAV, rayColorSRV), "Couldn't create ray color combo");

	//Output
	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, outputColorUAV[0], outputColorSRV[0]), "Couldn't create ray color combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(backBufferWidth, backBufferHeight, outputColorUAV[1], outputColorSRV[1]), "Couldn't create ray color combo");

	return true;
}

bool ConstantBufferShaderProgram::InitShaders()
{
	//////////////////////////////////////////////////
	//Cbuffers
	//////////////////////////////////////////////////
	//LogErrorReturnFalse(viewProjInverseBuffer.Create(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create view proj inverse buffer: ");

	//////////////////////////////////////////////////
	//Primary rays
	//////////////////////////////////////////////////
	ShaderResourceBinds primaryResourceBinds0;
	//CBuffers
	primaryResourceBinds0.AddResource(viewProjInverseBuffer, 0);

	//UAVs
	primaryResourceBinds0.AddResource(rayPositionUAV[0].get(), 0);
	primaryResourceBinds0.AddResource(rayDirectionUAV[0].get(), 1);
	primaryResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	primaryResourceBinds0.AddResource(outputColorUAV[1].get(), 3);

	LogErrorReturnFalse(primaryRayGenerator.CreateFromFile(shaderPath + "PrimaryRayGenerator.hlsl", device, primaryResourceBinds0), "");

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBinds0;
	//CBuffers
	traceResourceBinds0.AddResource(sphereBuffer, ConstantBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleVertexBuffer, ConstantBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleBuffer, ConstantBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBinds0.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds0.AddResource(rayColorUAV.get(), 3);

	//SRVs
	traceResourceBinds0.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionSRV[0].get(), 1);
	traceResourceBinds0.AddResource(objFile->GetMeshes().front().material.ambientTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds0.AddResource(SamplerStates::linearClamp, 0);

	ShaderResourceBinds traceResourceBinds1;
	//CBuffers
	traceResourceBinds1.AddResource(sphereBuffer, ConstantBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleVertexBuffer, ConstantBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleBuffer, ConstantBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBinds1.AddResource(rayPositionUAV[0].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionUAV[0].get(), 1);
	traceResourceBinds1.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds1.AddResource(rayColorUAV.get(), 3);

	//SRVs
	traceResourceBinds1.AddResource(rayPositionSRV[1].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionSRV[1].get(), 1);
	traceResourceBinds1.AddResource(objFile->GetMeshes().front().material.ambientTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds1.AddResource(SamplerStates::linearClamp, 0);

	LogErrorReturnFalse(traceShader.CreateFromFile(shaderPath + "Intersection.hlsl", device, traceResourceBinds0, traceResourceBinds1), "");

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds0;
	//CBuffers
	shadeResourceBinds0.AddResource(sphereBuffer, ConstantBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleVertexBuffer, ConstantBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleBuffer, ConstantBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(pointlightAttenuationBuffer, 4);
	shadeResourceBinds0.AddResource(cameraPositionBuffer, 5);

	//UAVs
	shadeResourceBinds0.AddResource(outputColorUAV[0].get(), 0);

	//SRVs
	shadeResourceBinds0.AddResource(rayPositionSRV[1].get(), 0);
	shadeResourceBinds0.AddResource(rayNormalSRV.get(), 1);
	shadeResourceBinds0.AddResource(rayColorSRV.get(), 2);
	shadeResourceBinds0.AddResource(outputColorSRV[1].get(), 3);

	ShaderResourceBinds shadeResourceBinds1;
	//CBuffers
	shadeResourceBinds1.AddResource(sphereBuffer, ConstantBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleVertexBuffer, ConstantBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleBuffer, ConstantBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(pointlightAttenuationBuffer, 4);
	shadeResourceBinds1.AddResource(cameraPositionBuffer, 5);

	//UAVs
	shadeResourceBinds1.AddResource(outputColorUAV[1].get(), 0);

	//SRVs
	shadeResourceBinds1.AddResource(rayPositionSRV[0].get(), 0);
	shadeResourceBinds1.AddResource(rayNormalSRV.get(), 1);
	shadeResourceBinds1.AddResource(rayColorSRV.get(), 2);
	shadeResourceBinds1.AddResource(outputColorSRV[0].get(), 3);

	LogErrorReturnFalse(intersectionShader.CreateFromFile(shaderPath + "Shading.hlsl", device, shadeResourceBinds0, shadeResourceBinds1), "");

	ShaderResourceBinds compositResourceBinds0;
	compositResourceBinds0.AddResource(backBufferUAV, 0);
	compositResourceBinds0.AddResource(outputColorSRV[0].get(), 0);

	ShaderResourceBinds compositResourceBinds1;
	compositResourceBinds1.AddResource(backBufferUAV, 0);
	compositResourceBinds1.AddResource(outputColorSRV[1].get(), 0);

	LogErrorReturnFalse(compositShader.CreateFromFile(shaderPath + "Composit.hlsl", device, compositResourceBinds0, compositResourceBinds1), "");

	return true;
}

void ConstantBufferShaderProgram::Update(std::chrono::nanoseconds delta)
{
	ShaderProgram::Update(delta);
}

std::map<std::string, double> ConstantBufferShaderProgram::Draw()
{
	auto xmViewProjInverse = DirectX::XMLoadFloat4x4(&viewProjMatrix);
	xmViewProjInverse = DirectX::XMMatrixInverse(nullptr, xmViewProjInverse);

	auto viewProjInverse = DirectX::XMStoreFloat4x4(xmViewProjInverse);
	viewProjInverseBuffer.Update(deviceContext, &viewProjInverse);

	cameraPositionBuffer.Update(deviceContext, &cameraPosition);

	d3d11Timer.Start();
	DrawRayPrimary();
	d3d11Timer.Stop("Primary");

	for(int i = 0; i < rayBounces; ++i)
	{
		DrawRayIntersection(i % 2);
		d3d11Timer.Stop("Intersect" + std::to_string(i));
		DrawRayShading(i % 2);
		d3d11Timer.Stop("Shade" + std::to_string(i));
	}

	DrawComposit((rayBounces + 1) % 2);

	return d3d11Timer.Stop();
}

void ConstantBufferShaderProgram::DrawRayPrimary()
{
	primaryRayGenerator.Bind(deviceContext);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	primaryRayGenerator.Unbind(deviceContext);
}

void ConstantBufferShaderProgram::DrawRayIntersection(int config)
{
	traceShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	traceShader.Unbind(deviceContext);
}

void ConstantBufferShaderProgram::DrawRayShading(int config)
{
	intersectionShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	intersectionShader.Unbind(deviceContext);
}

void ConstantBufferShaderProgram::DrawComposit(int config)
{
	compositShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	compositShader.Unbind(deviceContext);
}

void ConstantBufferShaderProgram::AddOBJ(const std::string& path, DirectX::XMFLOAT3 position)
{
	objFile = contentManager->Load<OBJFile>(path);
	if(objFile == nullptr)
		return;

	Mesh mesh = objFile->GetMeshes().front();

	if(mesh.vertices.size() > MAX_VERTICES)
		Logger::LogLine(LOG_TYPE::WARNING, path + " has " + std::to_string(mesh.vertices.size()) + " vertices which is larger than MAX_VERTICES (" + std::to_string(MAX_VERTICES) + "). Only the first " + std::to_string(MAX_VERTICES) + " will be used");

	if(triangleBufferData.triangleCount + mesh.indicies.size() / 3 > MAX_INDICIES)
	{
		Logger::LogLine(LOG_TYPE::WARNING, path + " has " + std::to_string(mesh.indicies.size()) + " indicies and the buffer currently contains " + std::to_string(triangleBufferData.triangleCount) + " indicies. Can't add the model's indicies since it would overflow the buffer");
		return;
	}

	int vertexOffset = 0;

	for(int i = 0; i < triangleBufferData.triangleCount; i++)
	{
		vertexOffset = std::max(vertexOffset, triangleBufferData.triangles.indicies[i].x + 1);
		vertexOffset = std::max(vertexOffset, triangleBufferData.triangles.indicies[i].y + 1);
		vertexOffset = std::max(vertexOffset, triangleBufferData.triangles.indicies[i].z + 1);
	}

	if(vertexOffset + mesh.vertices.size() > MAX_VERTICES)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "sword.obj has " + std::to_string(mesh.vertices.size()) + " vertices and the buffer currently contains " + std::to_string(mesh.vertices.size()) + " vertices. Can't add the model's vertices since it would overflow the buffer");
		return;
	}

	for(int i = 0, end = static_cast<int>(mesh.vertices.size()); i < end; ++i)
	{
		vertexBufferData.vertices[vertexOffset + i].position.x = mesh.vertices[i].position.x + position.x;
		vertexBufferData.vertices[vertexOffset + i].position.y = mesh.vertices[i].position.y + position.y;
		vertexBufferData.vertices[vertexOffset + i].position.z = mesh.vertices[i].position.z + position.z;

		int u = mesh.vertices[i].texCoord.x * 0xFFFF;
		int v = mesh.vertices[i].texCoord.y * 0xFFFF;

		vertexBufferData.vertices[vertexOffset + i].texCoord = ((u << 16) | v);
	}

	int triangleOffset = triangleBufferData.triangleCount;

	for(int i = 0, end = static_cast<int>(mesh.indicies.size()) / 3; i < end; ++i)
	{
		triangleBufferData.triangles.indicies[triangleOffset + i].x = vertexOffset + mesh.indicies[i * 3];
		triangleBufferData.triangles.indicies[triangleOffset + i].y = vertexOffset + mesh.indicies[i * 3 + 1];
		triangleBufferData.triangles.indicies[triangleOffset + i].z = vertexOffset + mesh.indicies[i * 3 + 2];
		triangleBufferData.triangles.indicies[triangleOffset + i].w = 0;

		++triangleBufferData.triangleCount;
	}
}

void ConstantBufferShaderProgram::AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color)
{
	if(sphereBufferData.sphereCount == MAX_SPHERES)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Tried to add more sphere than " + std::to_string(MAX_SPHERES) + " (MAX_SPHERES)");
		return;
	}

	sphereBufferData.spheres.position[sphereBufferData.sphereCount] = sphere;
	sphereBufferData.spheres.color[sphereBufferData.sphereCount] = color;
	++sphereBufferData.sphereCount;
}

std::string ConstantBufferShaderProgram::ReloadShadersInternal()
{
	throw std::logic_error("The method or operation is not implemented.");
}

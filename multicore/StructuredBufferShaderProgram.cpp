#include "StructuredBufferShaderProgram.h"

#include <DXLib/ShaderResourceBinds.h>
#include <DXLib/States.h>
#include <DXLib/OBJFile.h>

StructuredBufferShaderProgram::StructuredBufferShaderProgram()
	: primaryRayGenerator("main", "cs_5_0")
	, traceShader("main", "cs_5_0")
	, intersectionShader("main", "cs_5_0")
	, compositShader("main", "cs_5_0")
{}

bool StructuredBufferShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	if(!ShaderProgram::Init(device, deviceContext, backBufferWidth, backBufferHeight, console, contentManager))
		return false;

	dispatchX = backBufferWidth / 32;
	dispatchY = backBufferHeight / 16;

	//sphereBufferData.sphereCount = 0;
	//triangleBufferData.triangleCount = 0;

	shaderPath = "Shaders/StructuredBuffer/";

	return true;
}

bool StructuredBufferShaderProgram::InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV)
{
	if(!ShaderProgram::InitBuffers(depthBufferUAV, backBufferUAV))
		return false;

	if(!sphereBufferData.empty())
		LogErrorReturnFalse(sphereBuffer.Create<StructuredBufferSharedBuffers::Sphere>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(sphereBufferData.size()), sphereBufferData.empty() ? nullptr : &sphereBufferData[0]), "Couldn't create sphere buffer: ");

	LogErrorReturnFalse(triangleVertexBuffer.Create<StructuredBufferSharedBuffers::Vertex>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(vertexBufferData.size()), vertexBufferData.empty() ? nullptr : &vertexBufferData[0]), "Couldn't create triangle vertex buffer: ");
	LogErrorReturnFalse(triangleBuffer.Create<StructuredBufferSharedBuffers::Triangle>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(triangleBufferData.size()), triangleBufferData.empty() ? nullptr : &triangleBufferData[0]), "Couldn't create triangle index buffer: ");
	LogErrorReturnFalse(viewProjInverseBuffer.Create<DirectX::XMFLOAT4X4>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create triangle index buffer: ");

	if(!InitUAVSRV())
		return false;
	if(!InitShaders())
		return false;

	return true;
}

bool StructuredBufferShaderProgram::InitUAVSRV()
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

bool StructuredBufferShaderProgram::InitShaders()
{
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
	primaryResourceBinds0.AddResource(depthBufferUAV, 4);

	LogErrorReturnFalse(primaryRayGenerator.CreateFromFile(shaderPath + "PrimaryRayGenerator.hlsl", device, primaryResourceBinds0), "");

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBindInitial;
	//CBuffers
	traceResourceBindInitial.AddResource(sphereBuffer.GetSRV(), StructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBindInitial.AddResource(triangleVertexBuffer.GetSRV(), StructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBindInitial.AddResource(triangleBuffer.GetSRV(), StructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBindInitial.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBindInitial.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBindInitial.AddResource(rayNormalUAV.get(), 2);
	traceResourceBindInitial.AddResource(rayColorUAV.get(), 3);
	traceResourceBindInitial.AddResource(depthBufferUAV, 4);

	//SRVs
	traceResourceBindInitial.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBindInitial.AddResource(rayDirectionSRV[0].get(), 1);
	traceResourceBindInitial.AddResource(objFile->GetMeshes().front().material.diffuseTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBindInitial.AddResource(SamplerStates::linearClamp, 0);

	ShaderResourceBinds traceResourceBinds0;
	//CBuffers
	traceResourceBinds0.AddResource(sphereBuffer.GetSRV(), StructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleVertexBuffer.GetSRV(), StructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleBuffer.GetSRV(), StructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBinds0.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds0.AddResource(rayColorUAV.get(), 3);

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	traceResourceBinds0.AddResource(nullUAV, 4);

	//SRVs
	traceResourceBinds0.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionSRV[0].get(), 1);
	traceResourceBinds0.AddResource(objFile->GetMeshes().front().material.diffuseTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds0.AddResource(SamplerStates::linearClamp, 0);

	ShaderResourceBinds traceResourceBinds1;
	//CBuffers
	traceResourceBinds1.AddResource(sphereBuffer.GetSRV(), StructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleVertexBuffer.GetSRV(), StructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleBuffer.GetSRV(), StructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBinds1.AddResource(rayPositionUAV[0].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionUAV[0].get(), 1);
	traceResourceBinds1.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds1.AddResource(rayColorUAV.get(), 3);
	traceResourceBinds1.AddResource(nullUAV, 4);

	//SRVs
	traceResourceBinds1.AddResource(rayPositionSRV[1].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionSRV[1].get(), 1);
	traceResourceBinds1.AddResource(objFile->GetMeshes().front().material.diffuseTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds1.AddResource(SamplerStates::linearClamp, 0);

	LogErrorReturnFalse(traceShader.CreateFromFile(shaderPath + "Intersection.hlsl", device, traceResourceBindInitial, traceResourceBinds0, traceResourceBinds1), "");

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds0;
	//CBuffers
	shadeResourceBinds0.AddResource(sphereBuffer.GetSRV(), StructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleVertexBuffer.GetSRV(), StructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleBuffer.GetSRV(), StructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
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
	shadeResourceBinds1.AddResource(sphereBuffer.GetSRV(), StructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleVertexBuffer.GetSRV(), StructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleBuffer.GetSRV(), StructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
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

void StructuredBufferShaderProgram::Update(std::chrono::nanoseconds delta)
{
	ShaderProgram::Update(delta);
}

std::map<std::string, double> StructuredBufferShaderProgram::Draw()
{
	auto xmViewProjInverse = DirectX::XMLoadFloat4x4(&viewProjMatrix);
	xmViewProjInverse = DirectX::XMMatrixInverse(nullptr, xmViewProjInverse);

	auto viewProjInverse = DirectX::XMStoreFloat4x4(xmViewProjInverse);
	viewProjInverseBuffer.Update(deviceContext, &viewProjInverse);

	cameraPositionBuffer.Update(deviceContext, &cameraPosition);

	d3d11Timer.Start();
	DrawRayPrimary();
	d3d11Timer.Stop("Primary");

	DrawRayIntersection(0);
	d3d11Timer.Stop("Intersect0");
	DrawRayShading(0);
	d3d11Timer.Stop("Shade0");

	for(int i = 1; i < rayBounces; ++i)
	{
		DrawRayIntersection(i + (i % 2));
		d3d11Timer.Stop("Intersect" + std::to_string(i));
		DrawRayShading(i % 2);
		d3d11Timer.Stop("Shade" + std::to_string(i));
	}

	DrawComposit((rayBounces + 1) % 2);

	return d3d11Timer.Stop();
}

void StructuredBufferShaderProgram::DrawRayPrimary()
{
	primaryRayGenerator.Bind(deviceContext);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	primaryRayGenerator.Unbind(deviceContext);
}

void StructuredBufferShaderProgram::DrawRayIntersection(int config)
{
	traceShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	traceShader.Unbind(deviceContext);
}

void StructuredBufferShaderProgram::DrawRayShading(int config)
{
	intersectionShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	intersectionShader.Unbind(deviceContext);
}

void StructuredBufferShaderProgram::DrawComposit(int config)
{
	compositShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	compositShader.Unbind(deviceContext);
}

void StructuredBufferShaderProgram::AddOBJ(const std::string& path, DirectX::XMFLOAT3 position, float scale)
{
	objFile = contentManager->Load<OBJFile>(path);
	if(objFile == nullptr)
		return;

	Mesh mesh = objFile->GetMeshes().front();

	int vertexOffset = 0;

	for(const auto& triangle : triangleBufferData)
	{
		vertexOffset = std::max(vertexOffset, triangle.indicies.x + 1);
		vertexOffset = std::max(vertexOffset, triangle.indicies.y + 1);
		vertexOffset = std::max(vertexOffset, triangle.indicies.z + 1);
	}

	for(int i = 0, end = static_cast<int>(mesh.vertices.size()); i < end; ++i)
	{
		StructuredBufferSharedBuffers::Vertex newVertex;

		newVertex.position.x = mesh.vertices[i].position.x + position.x;
		newVertex.position.y = mesh.vertices[i].position.y + position.y;
		newVertex.position.z = mesh.vertices[i].position.z + position.z;

		int u = mesh.vertices[i].texCoord.x * 0xFFFF;
		int v = mesh.vertices[i].texCoord.y * 0xFFFF;

		newVertex.texCoord = (u << 16) | v;

		vertexBufferData.push_back(std::move(newVertex));
	}

	int triangleOffset = static_cast<int>(triangleBufferData.size());

	for(int i = 0, end = static_cast<int>(mesh.indicies.size()) / 3; i < end; ++i)
	{
		StructuredBufferSharedBuffers::Triangle newTriangle;

		newTriangle.indicies.x = vertexOffset + mesh.indicies[i * 3];
		newTriangle.indicies.y = vertexOffset + mesh.indicies[i * 3 + 1];
		newTriangle.indicies.z = vertexOffset + mesh.indicies[i * 3 + 2];
		newTriangle.indicies.w = 0;

		triangleBufferData.push_back(std::move(newTriangle));
	}
}

void StructuredBufferShaderProgram::AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color)
{
	StructuredBufferSharedBuffers::Sphere newSphere;

	newSphere.position = sphere;
	newSphere.color = color;

	sphereBufferData.push_back(std::move(newSphere));
}

std::string StructuredBufferShaderProgram::ReloadShadersInternal()
{
	throw std::logic_error("The method or operation is not implemented.");
}

#include "AABBStructuredBufferShaderProgram.h"

#include <DXLib/ShaderResourceBinds.h>
#include <DXLib/States.h>
#include <DXLib/OBJFile.h>

AABBStructuredBufferShaderProgram::AABBStructuredBufferShaderProgram()
	: primaryRayGenerator("main", "cs_5_0")
	, traceShader("main", "cs_5_0")
	, intersectionShader("main", "cs_5_0")
	, compositShader("main", "cs_5_0")
{}

bool AABBStructuredBufferShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	if(!ShaderProgram::Init(device, deviceContext, backBufferWidth, backBufferHeight, console, contentManager))
		return false;

	dispatchX = backBufferWidth / 32;
	dispatchY = backBufferHeight / 16;

	shaderPath = "Shaders/AABBStructuredBuffer/";

	return true;
}

bool AABBStructuredBufferShaderProgram::InitBuffers(ID3D11UnorderedAccessView* backBufferUAV)
{
	if(!ShaderProgram::InitBuffers(backBufferUAV))
		return false;

	if(!sphereBufferData.empty())
		LogErrorReturnFalse(sphereBuffer.Create<AABBStructuredBufferSharedBuffers::Sphere>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), static_cast<int>(sphereBufferData.size()), sphereBufferData.empty() ? nullptr : &sphereBufferData[0]), "Couldn't create sphere buffer: ");
	LogErrorReturnFalse(triangleVertexBuffer.Create<AABBStructuredBufferSharedBuffers::Vertex>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), static_cast<int>(vertexBufferData.size()), vertexBufferData.empty() ? nullptr : &vertexBufferData[0]), "Couldn't create triangle vertex buffer: ");
	LogErrorReturnFalse(triangleBuffer.Create<AABBStructuredBufferSharedBuffers::Triangle>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), static_cast<int>(triangleBufferData.size()), triangleBufferData.empty() ? nullptr : &triangleBufferData[0]), "Couldn't create triangle index buffer: ");
	LogErrorReturnFalse(modelsBuffer.Create<AABBStructuredBufferSharedBuffers::Model>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), static_cast<int>(modelsBufferData.size()), modelsBufferData.empty() ? nullptr : &modelsBufferData[0]), "Couldn't create model buffer: ");

	LogErrorReturnFalse(viewProjInverseBuffer.Create<DirectX::XMFLOAT4X4>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create view proj inverse buffer: ");

	if(!InitUAVSRV())
		return false;
	if(!InitShaders())
		return false;

	return true;
}

bool AABBStructuredBufferShaderProgram::InitUAVSRV()
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

bool AABBStructuredBufferShaderProgram::InitShaders()
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
	traceResourceBinds0.AddResource(sphereBuffer, AABBStructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleVertexBuffer, AABBStructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleBuffer, AABBStructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(modelsBuffer, AABBStructuredBufferSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

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
	traceResourceBinds1.AddResource(sphereBuffer, AABBStructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleVertexBuffer, AABBStructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleBuffer, AABBStructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(modelsBuffer, AABBStructuredBufferSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

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
	shadeResourceBinds0.AddResource(sphereBuffer, AABBStructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleVertexBuffer, AABBStructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleBuffer, AABBStructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(modelsBuffer, AABBStructuredBufferSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);
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
	shadeResourceBinds1.AddResource(sphereBuffer, AABBStructuredBufferSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleVertexBuffer, AABBStructuredBufferSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleBuffer, AABBStructuredBufferSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(modelsBuffer, AABBStructuredBufferSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);
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

void AABBStructuredBufferShaderProgram::Update(std::chrono::nanoseconds delta)
{
	ShaderProgram::Update(delta);
}

void AABBStructuredBufferShaderProgram::Draw()
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
		d3d11Timer.Stop("Trace" + std::to_string(i));
		DrawRayShading(i % 2);
		d3d11Timer.Stop("Shade" + std::to_string(i));
	}

	DrawComposit((rayBounces + 1) % 2);

	std::map<std::string, double> d3d11Times = d3d11Timer.Stop();

	float traceTime = 0.0f;
	float shadeTime = 0.0f;
	for(const auto& pair : d3d11Times)
	{
		if(pair.first.compare(0, 5, "Trace") == 0)
			traceTime += static_cast<float>(pair.second);
		else if(pair.first.compare(0, 5, "Shade") == 0)
			shadeTime += static_cast<float>(pair.second);
		else
			graph.AddValueToTrack(pair.first, static_cast<float>(pair.second));
	}

	graph.AddValueToTrack("Trace", traceTime);
	graph.AddValueToTrack("Shade", shadeTime);
}

void AABBStructuredBufferShaderProgram::DrawRayPrimary()
{
	primaryRayGenerator.Bind(deviceContext);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	primaryRayGenerator.Unbind(deviceContext);
}

void AABBStructuredBufferShaderProgram::DrawRayIntersection(int config)
{
	traceShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	traceShader.Unbind(deviceContext);
}

void AABBStructuredBufferShaderProgram::DrawRayShading(int config)
{
	intersectionShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	intersectionShader.Unbind(deviceContext);
}

void AABBStructuredBufferShaderProgram::DrawComposit(int config)
{
	compositShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	compositShader.Unbind(deviceContext);
}

void AABBStructuredBufferShaderProgram::AddOBJ(const std::string& path, DirectX::XMFLOAT3 position)
{
	objFile = contentManager->Load<OBJFile>(path);
	if(objFile == nullptr)
		return;

	DirectX::XMFLOAT3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	DirectX::XMFLOAT3 aabbMax(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

	auto xmAABBMin = DirectX::XMLoadFloat3(&aabbMin);
	auto xmAABBMax = DirectX::XMLoadFloat3(&aabbMax);

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
		AABBStructuredBufferSharedBuffers::Vertex newVertex;

		newVertex.position.x = mesh.vertices[i].position.x + position.x;
		newVertex.position.y = mesh.vertices[i].position.y + position.y;
		newVertex.position.z = mesh.vertices[i].position.z + position.z;

		int u = mesh.vertices[i].texCoord.x * 0xFFFF;
		int v = mesh.vertices[i].texCoord.y * 0xFFFF;

		newVertex.texCoord = (u << 16) | v;

		auto xmNewVertexPosition = DirectX::XMLoadFloat3(&newVertex.position);
		xmAABBMin = DirectX::XMVectorMin(xmNewVertexPosition, xmAABBMin);
		xmAABBMax = DirectX::XMVectorMax(xmNewVertexPosition, xmAABBMax);

		vertexBufferData.push_back(std::move(newVertex));
	}

	int triangleCount = static_cast<int>(triangleBufferData.size());

	for(int i = 0, end = static_cast<int>(mesh.indicies.size()) / 3; i < end; ++i)
	{
		AABBStructuredBufferSharedBuffers::Triangle newTriangle;

		newTriangle.indicies.x = vertexOffset + mesh.indicies[i * 3];
		newTriangle.indicies.y = vertexOffset + mesh.indicies[i * 3 + 1];
		newTriangle.indicies.z = vertexOffset + mesh.indicies[i * 3 + 2];
		newTriangle.indicies.w = 0;

		triangleBufferData.push_back(std::move(newTriangle));
	}

	AABBStructuredBufferSharedBuffers::Model newModel;
	DirectX::XMStoreFloat3(&newModel.aabb.min, xmAABBMin);
	DirectX::XMStoreFloat3(&newModel.aabb.max, xmAABBMax);

	newModel.beginIndex = triangleCount;
	newModel.endIndex = static_cast<int>(triangleBufferData.size()) - 1;

	modelsBufferData.push_back(std::move(newModel));
}

void AABBStructuredBufferShaderProgram::AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color)
{
	AABBStructuredBufferSharedBuffers::Sphere newSphere;

	newSphere.position = sphere;
	newSphere.color = color;

	sphereBufferData.push_back(std::move(newSphere));
}

std::string AABBStructuredBufferShaderProgram::ReloadShadersInternal()
{
	throw std::logic_error("The method or operation is not implemented.");
}

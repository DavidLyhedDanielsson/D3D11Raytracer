#include "SuperSampledShaderProgram.h"

#include <DXLib/ShaderResourceBinds.h>
#include <DXLib/States.h>
#include <DXLib/OBJFile.h>

#include <DXConsole/console.h>
#include <DXConsole/commandGetterSetter.h>

SuperSampledShaderProgram::SuperSampledShaderProgram()
	: primaryRayGenerator("main", "cs_5_0")
	, traceShader("main", "cs_5_0")
	, intersectionShader("main", "cs_5_0")
	, compositShader("main", "cs_5_0")
	, pickingShader("main", "cs_5_0")
	, pickPosition(-1, -1)
{}

bool SuperSampledShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	if(!ShaderProgram::Init(device, deviceContext, backBufferWidth, backBufferHeight, console, contentManager))
		return false;

	dispatchX = superSampleWidth / 32;
	dispatchY = superSampleHeight / 16;

	shaderPath = "Shaders/SuperSampled/";

	return true;
}

bool SuperSampledShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager, UINT superSampleCount)
{
	this->superSampleCount = superSampleCount;
	this->superSampleWidth = backBufferWidth * superSampleCount;
	this->superSampleHeight = backBufferHeight * superSampleCount;

	auto superSampleCountCommand = new CommandGetterSetter<UINT>("superSampleCount", std::bind(&SuperSampledShaderProgram::GetSuperSampleCount, this), std::bind(&SuperSampledShaderProgram::SetSuperSampleCount, this, std::placeholders::_1));
	if(!console->AddCommand(superSampleCountCommand))
		delete superSampleCountCommand;

	return Init(device, deviceContext, backBufferWidth, backBufferHeight, console, contentManager);
}

bool SuperSampledShaderProgram::InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV)
{
	if(!ShaderProgram::InitBuffers(depthBufferUAV, backBufferUAV))
		return false;

	if(!sphereBufferData.empty())
		LogErrorReturnFalse(sphereBuffer.Create<SuperSampledSharedBuffers::Sphere>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(sphereBufferData.size()), sphereBufferData.empty() ? nullptr : &sphereBufferData[0]), "Couldn't create sphere buffer: ");
	LogErrorReturnFalse(triangleVertexBuffer.Create<SuperSampledSharedBuffers::Vertex>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(vertexBufferData.size()), vertexBufferData.empty() ? nullptr : &vertexBufferData[0]), "Couldn't create triangle vertex buffer: ");
	LogErrorReturnFalse(triangleBuffer.Create<SuperSampledSharedBuffers::Triangle>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(triangleBufferData.size()), triangleBufferData.empty() ? nullptr : &triangleBufferData[0]), "Couldn't create triangle index buffer: ");
	LogErrorReturnFalse(modelsBuffer.Create<SuperSampledSharedBuffers::Model>(device, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), true, false, static_cast<int>(modelsBufferData.size()), modelsBufferData.empty() ? nullptr : &modelsBufferData[0]), "Couldn't create model buffer: ");

	LogErrorReturnFalse(viewProjInverseBuffer.Create<DirectX::XMFLOAT4X4>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create view proj inverse buffer: ");
	LogErrorReturnFalse(superSampleBuffer.Create<int>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, &superSampleCount), "Couldn't create view proj inverse buffer: ");

	LogErrorReturnFalse(pickingMousePositionBuffer.Create<DirectX::XMINT2>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create view proj inverse buffer: "); //TODO D3D11_USAGE_STATIC ???
	
	if(!sphereBufferData.empty()
		|| !triangleBufferData.empty())
		LogErrorReturnFalse(pickingHitDataBuffer.Create<PickingSharedBuffers::HitData>(device, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, true, true, static_cast<int>(sphereBufferData.size() + triangleBufferData.size())), "Couldn't create picking hit data buffer: ");

	if(!InitUAVSRV())
		return false;
	if(!InitShaders())
		return false;

	return true;
}

bool SuperSampledShaderProgram::InitUAVSRV()
{
	//////////////////////////////////////////////////
	//Ray trace
	//////////////////////////////////////////////////
	//Position
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayPositionUAV[0], rayPositionSRV[0]), "Couldn't create ray position combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayPositionUAV[1], rayPositionSRV[1]), "Couldn't create ray position combo");

	//Direction
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayDirectionUAV[0], rayDirectionSRV[0]), "Couldn't create ray direction combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayDirectionUAV[1], rayDirectionSRV[1]), "Couldn't create ray direction combo");

	//Normal
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayNormalUAV, rayNormalSRV), "Couldn't create ray normal combo");

	//Color
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, rayColorUAV, rayColorSRV), "Couldn't create ray color combo");

	//Output
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, outputColorUAV[0], outputColorSRV[0]), "Couldn't create ray color combo");

	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, outputColorUAV[1], outputColorSRV[1]), "Couldn't create ray color combo");

	//Upscaled depth buffer
	LogErrorReturnFalse(CreateUAVSRVCombo(superSampleWidth, superSampleHeight, depthBufferUAVUpscaled, depthBufferSRVUpscaled, DXGI_FORMAT_R32_FLOAT), "Couldn't create upsacled depth buffer UAV");

	return true;
}

bool SuperSampledShaderProgram::InitShaders()
{
	//////////////////////////////////////////////////
	//Primary rays
	//////////////////////////////////////////////////
	ShaderResourceBinds primaryResourceBinds0;
	//CBuffers
	primaryResourceBinds0.AddResource(viewProjInverseBuffer, 0);
	primaryResourceBinds0.AddResource(superSampleBuffer, 1);

	//UAVs
	primaryResourceBinds0.AddResource(rayPositionUAV[0].get(), 0);
	primaryResourceBinds0.AddResource(rayDirectionUAV[0].get(), 1);
	primaryResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	primaryResourceBinds0.AddResource(outputColorUAV[1].get(), 3);
	primaryResourceBinds0.AddResource(depthBufferUAVUpscaled.get(), 4);

	LogErrorReturnFalse(primaryRayGenerator.CreateFromFile(shaderPath + "PrimaryRayGenerator.hlsl", device, primaryResourceBinds0), "");

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBindInitial;
	//CBuffers
	traceResourceBindInitial.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBindInitial.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBindInitial.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	traceResourceBindInitial.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBindInitial.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBindInitial.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBindInitial.AddResource(rayNormalUAV.get(), 2);
	traceResourceBindInitial.AddResource(rayColorUAV.get(), 3);
	traceResourceBindInitial.AddResource(depthBufferUAVUpscaled.get(), 4);

	//SRVs
	traceResourceBindInitial.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBindInitial.AddResource(rayDirectionSRV[0].get(), 1);
	traceResourceBindInitial.AddResource(objFile->GetMeshes().front().material.ambientTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBindInitial.AddResource(SamplerStates::linearClamp, 0);

	ShaderResourceBinds traceResourceBinds0;
	//CBuffers
	traceResourceBinds0.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds0.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

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
	traceResourceBinds0.AddResource(objFile->GetMeshes().front().material.ambientTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds0.AddResource(SamplerStates::linearClamp, 0);

	ShaderResourceBinds traceResourceBinds1;
	//CBuffers
	traceResourceBinds1.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	traceResourceBinds1.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

	//UAVs
	traceResourceBinds1.AddResource(rayPositionUAV[0].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionUAV[0].get(), 1);
	traceResourceBinds1.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds1.AddResource(rayColorUAV.get(), 3);
	traceResourceBinds1.AddResource(nullUAV, 4);

	//SRVs
	traceResourceBinds1.AddResource(rayPositionSRV[1].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionSRV[1].get(), 1);
	traceResourceBinds1.AddResource(objFile->GetMeshes().front().material.ambientTexture->GetTextureResourceView(), 2);

	//Samplers
	traceResourceBinds1.AddResource(SamplerStates::linearClamp, 0);

	LogErrorReturnFalse(traceShader.CreateFromFile(shaderPath + "Intersection.hlsl", device, traceResourceBindInitial, traceResourceBinds0, traceResourceBinds1), "");

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds0;
	//CBuffers
	shadeResourceBinds0.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds0.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);
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
	shadeResourceBinds1.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(pointLightBuffer, POINT_LIGHT_BUFFER_REGISTRY_INDEX);
	shadeResourceBinds1.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);
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
	compositResourceBinds0.AddResource(depthBufferUAV, 1);

	compositResourceBinds0.AddResource(outputColorSRV[0].get(), 0);
	compositResourceBinds0.AddResource(depthBufferSRVUpscaled.get(), 1);

	compositResourceBinds0.AddResource(superSampleBuffer, 0);

	ShaderResourceBinds compositResourceBinds1;
	compositResourceBinds1.AddResource(backBufferUAV, 0);
	compositResourceBinds1.AddResource(depthBufferUAV, 1);

	compositResourceBinds1.AddResource(outputColorSRV[1].get(), 0);
	compositResourceBinds1.AddResource(depthBufferSRVUpscaled.get(), 1);

	compositResourceBinds1.AddResource(superSampleBuffer, 0);

	LogErrorReturnFalse(compositShader.CreateFromFile(shaderPath + "Composit.hlsl", device, compositResourceBinds0, compositResourceBinds1), "");

	//////////////////////////////////////////////////
	//Picking
	//////////////////////////////////////////////////
	ShaderResourceBinds pickingIntersectionBinds;
	pickingIntersectionBinds.AddResource(pickingMousePositionBuffer, 0);
	pickingIntersectionBinds.AddResource(pickingHitDataBuffer.GetUAV(), 0);
	pickingIntersectionBinds.AddResource(rayPositionSRV[0].get(), 0);
	pickingIntersectionBinds.AddResource(rayDirectionSRV[0].get(), 1);

	pickingIntersectionBinds.AddResource(sphereBuffer.GetSRV(), SuperSampledSharedBuffers::SPHERE_BUFFER_REGISTRY_INDEX);
	pickingIntersectionBinds.AddResource(triangleVertexBuffer.GetSRV(), SuperSampledSharedBuffers::VERTEX_BUFFER_REGISTRY_INDEX);
	pickingIntersectionBinds.AddResource(triangleBuffer.GetSRV(), SuperSampledSharedBuffers::TRIANGLE_BUFFER_REGISTRY_INDEX);
	pickingIntersectionBinds.AddResource(modelsBuffer.GetSRV(), SuperSampledSharedBuffers::MODEL_BUFFER_REGISTRY_INDEX);

	LogErrorReturnFalse(pickingShader.CreateFromFile("Shaders/Picking/Intersection.hlsl", device, pickingIntersectionBinds), "");

	return true;
}

void SuperSampledShaderProgram::Update(std::chrono::nanoseconds delta)
{
	ShaderProgram::Update(delta);
}

std::map<std::string, double> SuperSampledShaderProgram::Draw()
{
	auto xmViewProjInverse = DirectX::XMLoadFloat4x4(&viewProjMatrix);
	xmViewProjInverse = DirectX::XMMatrixInverse(nullptr, xmViewProjInverse);

	auto viewProjInverse = DirectX::XMStoreFloat4x4(xmViewProjInverse);
	viewProjInverseBuffer.Update(deviceContext, &viewProjInverse);

	cameraPositionBuffer.Update(deviceContext, &cameraPosition);
	superSampleBuffer.Update(deviceContext, &superSampleCount);

	d3d11Timer.Start();
	DrawRayPrimary();
	d3d11Timer.Stop("Primary");

	//Do picking here "for free"
	//if(pickPosition.x != -1)
	//{
	if(pickingCallback != nullptr)
	{
		pickPosition.x *= superSampleCount;
		pickPosition.y *= superSampleCount;

		pickPosition.x = 640;
		pickPosition.y = 360;

		//Logger::LogLine(LOG_TYPE::INFO, "Picking at " + std::to_string(pickPosition.x) + ", " + std::to_string(pickPosition.y));

		pickingMousePositionBuffer.Update(deviceContext, &pickPosition);
		pickPosition = DirectX::XMINT2(-1, -1);

		DrawPick();
	}
	//}

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

void SuperSampledShaderProgram::DrawRayPrimary()
{
	primaryRayGenerator.Bind(deviceContext);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	primaryRayGenerator.Unbind(deviceContext);
}

void SuperSampledShaderProgram::DrawRayIntersection(int config)
{
	traceShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	traceShader.Unbind(deviceContext);
}

void SuperSampledShaderProgram::DrawRayShading(int config)
{
	intersectionShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	intersectionShader.Unbind(deviceContext);
}

void SuperSampledShaderProgram::DrawComposit(int config)
{
	//Downscales a superSampleCount x superSampleCount grid, so dispatch fewer groups
	compositShader.Bind(deviceContext, config);
	deviceContext->Dispatch(dispatchX / superSampleCount, dispatchY / superSampleCount, 1);
	compositShader.Unbind(deviceContext);
}

void SuperSampledShaderProgram::DrawPick()
{
	pickingShader.Bind(deviceContext);
	deviceContext->Dispatch(static_cast<int>(std::ceil((sphereBufferData.size() + triangleBufferData.size()) / 32.0f)), 1, 1);
	pickingShader.Unbind(deviceContext);

	std::vector<PickingSharedBuffers::HitData> hitData;
	hitData.resize(sphereBufferData.size() + triangleBufferData.size());

	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	deviceContext->Map(pickingHitDataBuffer.GetBuffer(), 0, D3D11_MAP_READ, 0, &mappedBuffer);
	memcpy(&hitData[0], mappedBuffer.pData, sizeof(PickingSharedBuffers::HitData) * (sphereBufferData.size() + triangleBufferData.size()));
	deviceContext->Unmap(pickingHitDataBuffer.GetBuffer(), 0);

	float nearest = std::numeric_limits<float>::max();
	int nearestIndex = -1;

	for(int i = 0, end = static_cast<int>(hitData.size()); i < end; ++i)
	{
		if(hitData[i].depth >= 0.0f
			&& hitData[i].depth < nearest)
		{
			nearest = hitData[i].depth;
			nearestIndex = i;
		}
	}

	PickedObjectData data;
	
	if(nearestIndex == -1)
	{
		data.modelIndex = -1;
		data.triangleIndex = -1;
		data.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		data.color = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	}
	else
	{
		data.modelIndex = hitData[nearestIndex].modelIndex;
		data.triangleIndex = hitData[nearestIndex].triangleIndex;

		if(data.triangleIndex == -1)
		{
			data.position = DirectX::XMLoadFloat3(sphereBufferData[nearestIndex].position);
			data.color = DirectX::XMLoadFloat3(sphereBufferData[nearestIndex].color);
		}
		else
		{
			data.position = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f);
			data.color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f);
		}
	}

	pickingCallback(data);
}

void SuperSampledShaderProgram::Pick(const DirectX::XMINT2& mousePosition, std::function<void(const PickedObjectData&)> callback)
{
	if(pickingHitDataBuffer.GetUAV() == nullptr)
		return;

	ShaderProgram::Pick(mousePosition, callback);

	pickPosition = mousePosition;
}

void SuperSampledShaderProgram::AddOBJ(const std::string& path, DirectX::XMFLOAT3 position)
{
	objFile = contentManager->Load<OBJFile>(path);
	if(objFile == nullptr)
		return;

	DirectX::XMFLOAT3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	DirectX::XMFLOAT3 aabbMax(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

	auto xmAABBMin = DirectX::XMLoadFloat3(&aabbMin);
	auto xmAABBMax = DirectX::XMLoadFloat3(&aabbMax);

	int vertexOffset = 0;

	for(const auto& triangle : triangleBufferData)
	{
		vertexOffset = std::max(vertexOffset, triangle.indicies.x + 1);
		vertexOffset = std::max(vertexOffset, triangle.indicies.y + 1);
		vertexOffset = std::max(vertexOffset, triangle.indicies.z + 1);
	}

	for(const auto& mesh : objFile->GetMeshes())
	{
		for(int i = 0, end = static_cast<int>(mesh.vertices.size()); i < end; ++i)
		{
			SuperSampledSharedBuffers::Vertex newVertex;

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
			SuperSampledSharedBuffers::Triangle newTriangle;

			newTriangle.indicies.x = vertexOffset + mesh.indicies[i * 3];
			newTriangle.indicies.y = vertexOffset + mesh.indicies[i * 3 + 1];
			newTriangle.indicies.z = vertexOffset + mesh.indicies[i * 3 + 2];
			newTriangle.indicies.w = 0;

			triangleBufferData.push_back(std::move(newTriangle));
		}

		SuperSampledSharedBuffers::Model newModel;
		DirectX::XMStoreFloat3(&newModel.aabb.min, xmAABBMin);
		DirectX::XMStoreFloat3(&newModel.aabb.max, xmAABBMax);

		newModel.beginIndex = triangleCount;
		newModel.endIndex = static_cast<int>(triangleBufferData.size());

		modelsBufferData.push_back(std::move(newModel));
	}
}

void SuperSampledShaderProgram::SetSuperSampleCount(UINT count)
{
	if(superSampleCount == count)
		return;

	superSampleCount = count;
	superSampleWidth = backBufferWidth * superSampleCount;
	superSampleHeight = backBufferHeight * superSampleCount;

	dispatchX = superSampleWidth / 32;
	dispatchY = superSampleHeight / 16;

	InitUAVSRV();
	InitShaders();
}

UINT SuperSampledShaderProgram::GetSuperSampleCount() const
{
	return superSampleCount;
}

void SuperSampledShaderProgram::AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color)
{
	SuperSampledSharedBuffers::Sphere newSphere;

	newSphere.position = sphere;
	newSphere.color = color;

	sphereBufferData.push_back(std::move(newSphere));
}

std::string SuperSampledShaderProgram::ReloadShadersInternal()
{
	throw std::logic_error("The method or operation is not implemented.");
}

#include "MulticoreWindow.h"

#include <thread>
#include <vector>
#include <functional>

#include <DXLib/Logger.h>
#include <DXLib/input.h>
#include <DXLib/States.h>

#include <DXConsole/console.h>

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, billboardSamplerState(nullptr)
	, backbufferUAV(nullptr)
	, vertexBuffer(nullptr)
	, indexBuffer(nullptr)
	, viewProjMatrixBuffer(nullptr)
	, viewProjInverseBuffer(nullptr)
	, pointLightBuffer(nullptr)
	, sphereBuffer(nullptr)
	, triangleBuffer(nullptr)
	, rayNormalUAV(nullptr)
	, rayNormalSRV(nullptr)
	, rayDirectionUAV{ COMUniquePtr<ID3D11UnorderedAccessView>(nullptr), COMUniquePtr<ID3D11UnorderedAccessView>(nullptr) }
	, rayPositionUAV{ COMUniquePtr<ID3D11UnorderedAccessView>(nullptr), COMUniquePtr<ID3D11UnorderedAccessView>(nullptr) }
	, rayDirectionSRV{ COMUniquePtr<ID3D11ShaderResourceView>(nullptr), COMUniquePtr<ID3D11ShaderResourceView>(nullptr) }
	, rayPositionSRV{ COMUniquePtr<ID3D11ShaderResourceView>(nullptr), COMUniquePtr<ID3D11ShaderResourceView>(nullptr) }
	, bulbProjMatrixBuffer(nullptr)
	, bulbInstanceBuffer(nullptr)
	, bulbVertexBuffer(nullptr)
	, billboardBlendState(nullptr)
	, bulbTexture(nullptr)
	, primaryRayGenerator("main", "cs_5_0")
	, trace("main", "cs_5_0")
	, shade("main", "cs_5_0")
	, billboardVertexShader("main", "vs_5_0")
	, billboardPixelShader("main", "ps_5_0")
	, drawConsole(false)
{
}

MulticoreWindow::~MulticoreWindow()
{}

bool MulticoreWindow::Init()
{
	srand(time(NULL));

	//Content manager
	contentManager.Init(device.get());
	calibri16 = contentManager.Load<CharacterSet>("calibri16");

	//Sprite renderer
	spriteRenderer.Init(device.get(), deviceContext.get(), &contentManager, 1280, 720);

	InitConsole();
	InitInput();

	if(!InitSRVs())
		return false;
	if(!InitRoom())
		return false;
	if(!InitPointLights())
		return false;
	if(!InitRaytraceShaders())
		return false;

	if(!InitBulb())
		return false;
	if(!InitGraphs())
		return false;

	//Etc
	camera.InitFovHorizontal(DirectX::XMFLOAT3(0.0f, 0.0f, -3.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XM_PIDIV2, 1280.0f / 720.0f, 0.01f, 100.0f);

	POINT midPoint;
	midPoint.x = 640;
	midPoint.y = 360;

	ClientToScreen(hWnd, &midPoint);
	SetCursorPos(midPoint.x, midPoint.y);

	return true;
}

void MulticoreWindow::Run()
{
	if(!Init())
		return;

	std::chrono::nanoseconds accumulatedDelta(0);
	int iterations = 0;

	bool run = true;

	Timer updateTimer;
	Timer drawTimer;

	gameTimer.Start();
	gameTimer.UpdateDelta();
	while(run)
	{
		run = PeekMessages();
		Input::Update();

		if(!paused)
		{
			gameTimer.UpdateDelta();

			updateTimer.Reset();
			updateTimer.Start();
			Update(gameTimer.GetDelta());
			updateTimer.Stop();

			drawTimer.Reset();
			drawTimer.Start();
			Draw();
			drawTimer.Stop();

			cpuGraph.AddValueToTrack("Update", updateTimer.GetTimeMillisecondsFraction());
			cpuGraph.AddValueToTrack("Draw", drawTimer.GetTimeMillisecondsFraction());
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

float sinVal = 0.0f;

void MulticoreWindow::Update(std::chrono::nanoseconds delta)
{
	double deltaMS = delta.count() * 1e-6;

	sinVal += deltaMS * 0.0005f;

	if(!drawConsole)
	{
		POINT cursorPosition;
		GetCursorPos(&cursorPosition);
		ScreenToClient(hWnd, &cursorPosition);

		POINT midPoint;
		midPoint.x = 640;
		midPoint.y = 360;

		float xDelta = cursorPosition.x - midPoint.x;
		float yDelta = cursorPosition.y - midPoint.y;

		float sensitivity = 0.001f;

		camera.Rotate(DirectX::XMFLOAT2(xDelta * sensitivity, yDelta * sensitivity));

		ClientToScreen(hWnd, &midPoint);
		SetCursorPos(midPoint.x, midPoint.y);

		float cameraSpeed = 0.005f * deltaMS;

		if(keyMap.count('W'))
			camera.MoveFoward(cameraSpeed);
		else if(keyMap.count('S'))
			camera.MoveFoward(-cameraSpeed);

		if(keyMap.count('A'))
			camera.MoveRight(-cameraSpeed);
		else if(keyMap.count('D'))
			camera.MoveRight(cameraSpeed);

		if(keyMap.count(VK_SPACE))
			camera.MoveUp(cameraSpeed);
		else if(keyMap.count(VK_CONTROL))
			camera.MoveUp(-cameraSpeed);

		camera.CalcViewMatrix();
	}

	guiManager.Update(delta);
}

void MulticoreWindow::Draw()
{
	float colors[] = { 44.0f / 255.0f, 87.0f / 255.0f, 120.0f / 255.0f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRenderTarget.get(), colors);

	//Update viewProjMatrix and viewProjInverseMatrix
	DirectX::XMFLOAT4X4 viewMatrix = camera.GetViewMatrix();
	DirectX::XMFLOAT4X4 projectionMatrix = camera.GetProjectionMatrix();
	DirectX::XMFLOAT4X4 viewProjectionMatrix;

	DirectX::XMMATRIX xmViewMatrix = DirectX::XMLoadFloat4x4(&viewMatrix);
	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMLoadFloat4x4(&projectionMatrix);

	DirectX::XMMATRIX xmViewProjMatrix = DirectX::XMMatrixMultiplyTranspose(xmViewMatrix, xmProjectionMatrix);

	ViewProjBuffer viewProjBuffer;

	DirectX::XMStoreFloat4x4(&viewProjectionMatrix, xmViewProjMatrix);
	DirectX::XMStoreFloat4x4(&viewProjBuffer.viewProjMatrixInverse, DirectX::XMMatrixInverse(nullptr, xmViewProjMatrix));

	D3D11_MAPPED_SUBRESOURCE mappedViewProjMatrixBuffer;
	if(FAILED(deviceContext->Map(viewProjInverseBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedViewProjMatrixBuffer)))
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't map buffer");
		return;
	}

	std::memcpy(mappedViewProjMatrixBuffer.pData, &viewProjBuffer, sizeof(float) * 32);

	deviceContext->Unmap(viewProjInverseBuffer.get(), 0);

	d3d11Timer.Start();

	//////////////////////////////////////////////////
	//Rays
	//////////////////////////////////////////////////
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	//////////////////////////////////////////////////
	//Primary
	//////////////////////////////////////////////////
	primaryRayGenerator.Bind(deviceContext.get());
	deviceContext->Dispatch(40, 45, 1);
	primaryRayGenerator.Unbind(deviceContext.get());

	d3d11Timer.Stop("Primary");

	//////////////////////////////////////////////////
	//Trace
	//////////////////////////////////////////////////

	trace.Bind(deviceContext.get());
	deviceContext->Dispatch(40, 45, 1);
	trace.Unbind(deviceContext.get());

	d3d11Timer.Stop("Trace");

	pointLightBufferData.lights[0].y = (1.0f + std::sinf(sinVal)) * 4.0f + 0.5f;
	pointLightBufferData.lights[1].y = (1.0f + std::sinf(sinVal)) * 4.0f + 0.5f;
	pointLightBufferData.lights[2].y = (1.0f + std::sinf(sinVal)) * 4.0f + 0.5f;
	pointLightBufferData.lights[3].y = (1.0f + std::sinf(sinVal)) * 4.0f + 0.5f;

	D3D11_MAPPED_SUBRESOURCE mappedLightBuffer;
	deviceContext->Map(pointLightBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedLightBuffer);
	memcpy(mappedLightBuffer.pData, &pointLightBufferData, sizeof(PointlightBuffer));
	deviceContext->Unmap(pointLightBuffer.get(), 0);

	shade.Bind(deviceContext.get());
	deviceContext->Dispatch(40, 45, 1);
	shade.Unbind(deviceContext.get());

	d3d11Timer.Stop("Shade");

	std::map<std::string, double> d3d11Times = d3d11Timer.Stop();
	for(const auto& pair : d3d11Times)
		gpuGraph.AddValueToTrack(pair.first, static_cast<float>(pair.second));

	//////////////////////////////////////////////////
	//Forward rendering
	//////////////////////////////////////////////////
	renderTargets[0] = backBufferRenderTarget.get();
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	deviceContext->ClearDepthStencilView(depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	deviceContext->RSSetState(rasterizerState.get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//////////////////////////////////////////////////
	//Bulbs
	//////////////////////////////////////////////////
	for(int i = 0; i < pointLightBufferData.lightCount; i++)
	{
		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixTranslation(pointLightBufferData.lights[i].x, pointLightBufferData.lights[i].y, pointLightBufferData.lights[i].z);
		DirectX::XMStoreFloat4x4(&bulbInstanceData[i].worldMatrix, worldMatrix);
	}

	D3D11_MAPPED_SUBRESOURCE mappedBulbBuffer;
	ZeroMemory(&mappedBulbBuffer, sizeof(mappedBulbBuffer));
	if(FAILED(deviceContext->Map(bulbInstanceBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBulbBuffer)))
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't map buffer");
		return;
	}
	memcpy(mappedBulbBuffer.pData, &bulbInstanceData[0], sizeof(BulbInstanceData) * MAX_POINT_LIGHTS);
	deviceContext->Unmap(bulbInstanceBuffer.get(), 0);

	D3D11_MAPPED_SUBRESOURCE mappedBulbMatrixBuffer;
	ZeroMemory(&mappedBulbMatrixBuffer, sizeof(mappedBulbMatrixBuffer));
	if(FAILED(deviceContext->Map(bulbProjMatrixBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBulbMatrixBuffer)))
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't map buffer");
		return;
	}
	memcpy(mappedBulbMatrixBuffer.pData, &viewProjectionMatrix, sizeof(float) * 16);
	deviceContext->Unmap(bulbProjMatrixBuffer.get(), 0);

	ID3D11Buffer* bulbBuffers[] = { bulbProjMatrixBuffer.get(), bulbInstanceBuffer.get() };
	deviceContext->VSSetConstantBuffers(0, 2, bulbBuffers);

	UINT bulbStride = sizeof(Vertex);
	UINT bulbOffset = 0;

	ID3D11Buffer* bulbVertexBuffers[] = { bulbVertexBuffer.get() };
	deviceContext->IASetVertexBuffers(0, 1, bulbVertexBuffers, &bulbStride, &bulbOffset);

	ID3D11ShaderResourceView* bulbTextureDumb = bulbTexture->GetTextureResourceView();
	deviceContext->PSSetShaderResources(0, 1, &bulbTextureDumb);

	deviceContext->PSSetSamplers(0, 1, &billboardSamplerState);

	float blendFac[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	deviceContext->OMSetBlendState(billboardBlendState, blendFac, 0xFFFFFFFF);
	deviceContext->RSSetState(billboardRasterizerState);

	billboardVertexShader.Bind(deviceContext.get());
	billboardPixelShader.Bind(deviceContext.get());
	deviceContext->DrawInstanced(6, pointLightBufferData.lightCount, 0, 0);
	billboardVertexShader.Unbind(deviceContext.get());
	billboardPixelShader.Unbind(deviceContext.get());

	//////////////////////////////////////////////////
	//Sprites
	//////////////////////////////////////////////////
	spriteRenderer.Begin();

	guiManager.Draw(&spriteRenderer);

	gpuGraph.Draw(&spriteRenderer);
	cpuGraph.Draw(&spriteRenderer);

	spriteRenderer.End();

	gpuGraph.Draw();
	cpuGraph.Draw();

	swapChain->Present(1, 0);
}

LRESULT MulticoreWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_ACTIVATE:
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				paused = true;
				gameTimer.Stop();
			}
			else
			{
				paused = false;
				gameTimer.Start();
			}
			break;
		default:
			return DX11Window::WndProc(hWnd, msg, wParam, lParam);
			break;
	}

	return 0;
}

void MulticoreWindow::KeyEvent(const KeyState& keyCode)
{
	if(keyCode.key == VK_ESCAPE)
		PostQuitMessage(0);

	if(keyCode.action == KEY_ACTION::DOWN)
	{
		keyMap.insert(keyCode.key);

		if(keyCode.key == VK_OEM_5)
		{
			drawConsole = !drawConsole;

			if(drawConsole)
				console.Activate();
			else
				console.Deactivate();
		}
	}
	else if(keyCode.action == KEY_ACTION::UP)
		keyMap.erase(keyCode.key);

	guiManager.KeyEvent(keyCode);
}

void MulticoreWindow::MouseEvent(const KeyState& keyCode)
{
	guiManager.MouseEvent(keyCode);
}

void MulticoreWindow::CharEvent(int character)
{
	guiManager.CharEvent(character);
}

void MulticoreWindow::ScrollEvent(int distance)
{
	guiManager.ScrollEvent(distance);
}

bool MulticoreWindow::InitSRVs()
{
	//////////////////////////////////////////////////
	//Back buffer SRV
	//////////////////////////////////////////////////
	ID3D11Texture2D* backBufferDumb = nullptr;
	HRESULT hRes = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferDumb));
	COMUniquePtr<ID3D11Texture2D> backBuffer(backBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't get backBuffer from swapChain");
		return false;
	}

	ID3D11UnorderedAccessView* backBufferUAVDumb = nullptr;
	device->CreateUnorderedAccessView(backBuffer.get(), nullptr, &backBufferUAVDumb);
	backbufferUAV.reset(backBufferUAVDumb);
	if(backbufferUAV == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create back buffer UAV");
		return false;
	}

	//////////////////////////////////////////////////
	//Rays
	//////////////////////////////////////////////////
	//Position
	if(!CreateUAVSRVCombo(width, height, rayPositionUAV[0], rayPositionSRV[0]))
		return false;

	if(!CreateUAVSRVCombo(width, height, rayPositionUAV[1], rayPositionSRV[1]))
		return false;

	//Direction
	if(!CreateUAVSRVCombo(width, height, rayDirectionUAV[0], rayDirectionSRV[0]))
		return false;

	if(!CreateUAVSRVCombo(width, height, rayDirectionUAV[1], rayDirectionSRV[1]))
		return false;

	//Normal
	if(!CreateUAVSRVCombo(width, height, rayNormalUAV, rayNormalSRV))
		return false;

	return true;
}

bool MulticoreWindow::InitRaytraceShaders()
{
	//////////////////////////////////////////////////
	//Cbuffers
	//////////////////////////////////////////////////
	viewProjMatrixBuffer = CreateBuffer(BUFFER_DATA_TYPES::MAT4X4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE);
	if(viewProjMatrixBuffer == nullptr)
		return false;

	viewProjInverseBuffer = CreateBuffer(BUFFER_DATA_TYPES::MAT4X4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE);
	if(viewProjInverseBuffer == nullptr)
		return false;

	//////////////////////////////////////////////////
	//Primary rays
	//////////////////////////////////////////////////
	ShaderResourceBinds primaryResourceBinds;
	primaryResourceBinds.AddResource(rayPositionUAV[0].get(), 0);
	primaryResourceBinds.AddResource(rayDirectionUAV[0].get(), 1);
	primaryResourceBinds.AddResource(rayNormalUAV.get(), 2);
	primaryResourceBinds.AddResource(viewProjInverseBuffer.get(), 0);

	std::string errorString = primaryRayGenerator.CreateFromFile("PrimaryRayGenerator.cso", device.get(), primaryResourceBinds);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBinds;
	traceResourceBinds.AddResource(sphereBuffer.get(), 0);
	traceResourceBinds.AddResource(triangleBuffer.get(), 1);
	traceResourceBinds.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBinds.AddResource(rayNormalUAV.get(), 1);
	traceResourceBinds.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBinds.AddResource(rayDirectionSRV[0].get(), 1);

	errorString = trace.CreateFromFile("Trace.cso", device.get(), traceResourceBinds);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds;

	shadeResourceBinds.AddResource(backbufferUAV.get(), 0);
	shadeResourceBinds.AddResource(rayPositionSRV[1].get(), 0);
	shadeResourceBinds.AddResource(rayNormalSRV.get(), 1);
	shadeResourceBinds.AddResource(pointLightBuffer.get(), 0);
	shadeResourceBinds.AddResource(sphereBuffer.get(), 1);
	shadeResourceBinds.AddResource(triangleBuffer.get(), 2);

	errorString = shade.CreateFromFile("Shading.cso", device.get(), shadeResourceBinds);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}
}

bool MulticoreWindow::InitBulb()
{
	std::string errorString = billboardPixelShader.CreateFromFile("BillboardPixelShader.cso", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	errorString = billboardVertexShader.CreateFromFile("BillboardVertexShader.cso", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	billboardVertexShader.SetVertexData(device.get(),
		std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4X4 }
	, std::vector<std::string> { "POSITION", "COLOR", "TEX_COORD", "WORLD_MATRIX" }
	, std::vector<bool> { false, false, false, true });

	bulbInstanceBuffer = CreateBuffer(sizeof(BulbInstanceData) * MAX_POINT_LIGHTS, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, nullptr);
	bulbProjMatrixBuffer = CreateBuffer(BUFFER_DATA_TYPES::MAT4X4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, nullptr);

	std::vector<Vertex> bulbVertices;
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	bulbVertexBuffer = CreateBuffer(sizeof(Vertex) * 6, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &bulbVertices[0]);

	bulbTexture = contentManager.Load<Texture2D>("Bulb.dds");
	if(bulbTexture == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't load bulb texture");
		return false;
	}

	billboardBlendState = BlendStates::singleDefault;
	billboardSamplerState = SamplerStates::linearClamp;
	billboardRasterizerState = RasterizerStates::solid;
}

bool MulticoreWindow::InitPointLights()
{
	pointLightBufferData.lights[0] = DirectX::XMFLOAT4(0.0f, 0.0f, -5.0f, 10.0f);
	pointLightBufferData.lights[1] = DirectX::XMFLOAT4(0.0f, 0.0f, 5.0f, 10.0f);
	pointLightBufferData.lights[2] = DirectX::XMFLOAT4(-5.0f, 0.0f, 0.0f, 10.0f);
	pointLightBufferData.lights[3] = DirectX::XMFLOAT4(5.0f, 0.0f, 0.0f, 10.0f);
	pointLightBufferData.lightCount = 4;

	pointLightBuffer = CreateBuffer(sizeof(float) * 4 * MAX_POINT_LIGHTS + sizeof(int) * 4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, &pointLightBufferData);
	if(pointLightBuffer == nullptr)
		return false;

	return true;
}

bool MulticoreWindow::InitGraphs()
{
	//////////////////////////////////////////////////
	//Timing
	//////////////////////////////////////////////////
	//CPU
#ifdef _DEBUG
	int cpuMSAverage = 10;
#else
	int cpuMSAverage = 50;
#endif
	std::vector<Track> cpuTracks{ Track(cpuMSAverage, 1.0f) };
	std::vector<std::string> cpuTrackNames{ "Update", "Draw" };

	std::string errorString = cpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(0, 720 - 128), DirectX::XMINT2(256, 128), 30.0f, 1);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize CPU graph: " + errorString);
		return false;
	}

	errorString = cpuGraph.AddTracks(cpuTrackNames, cpuTracks);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't add CPU tracks to graph: " + errorString);
		return false;
	}

	//GPU
#ifdef _DEBUG
	int gpuMSAverage = 10;
#else
	int gpuMSAverage = 10;
#endif

	std::vector<Track> gpuTracks;
	std::vector<std::string> gpuTrackNames;

	std::vector<std::string> timerQueries{ "Primary", "Trace", "Shade" };

	if(!d3d11Timer.Init(device.get(), deviceContext.get(), timerQueries))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize d3d11Timer");
		return false;
	}

	gpuTracks.reserve(gpuTracks.size() + timerQueries.size());
	for(std::string& name : timerQueries)
	{
		gpuTrackNames.emplace_back(std::move(name));
		gpuTracks.emplace_back(gpuMSAverage, 1.0f);
	}

	errorString = gpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(256 + cpuGraph.GetBackgroundWidth(), 720 - 128), DirectX::XMINT2(256, 128), 10.0f, 1);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize GPU graph: " + errorString);
		return false;
	}

	errorString = gpuGraph.AddTracks(gpuTrackNames, gpuTracks);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't add GPU tracks to graph: " + errorString);
		return false;
	}
}

bool MulticoreWindow::InitRoom()
{
	SphereBuffer sphereBufferData;
	sphereBufferData.sphereCount = 5;
	sphereBufferData.spheres[0] = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 2.5f);
	sphereBufferData.spheres[1] = DirectX::XMFLOAT4(0.0f, 1.0f, -2.0f, 1.0f);
	sphereBufferData.spheres[2] = DirectX::XMFLOAT4(0.0f, 1.0f, 2.0f, 1.0f);
	sphereBufferData.spheres[3] = DirectX::XMFLOAT4(-2.0f, 1.0f, 0.0f, 1.0f);
	sphereBufferData.spheres[4] = DirectX::XMFLOAT4(2.0f, 1.0f, 0.0f, 1.0f);

	TriangleBuffer triangleBufferData;
	triangleBufferData.triangleCount = 12;

	float height = 10.0f;
	float size = 12.0f;

	//Floor
	triangleBufferData.triangles[0] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[1] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[2] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	triangleBufferData.triangles[3] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[4] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[5] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	//Ceiling
	triangleBufferData.triangles[6] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.triangles[7] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.triangles[8] = DirectX::XMFLOAT4(size, height, -size, 0.0f);

	triangleBufferData.triangles[9] = DirectX::XMFLOAT4(-size, height, size, 0.0f);
	triangleBufferData.triangles[10] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.triangles[11] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);

	//z+ wall
	triangleBufferData.triangles[12] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[13] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[14] = DirectX::XMFLOAT4(-size, height, size, 0.0f);

	triangleBufferData.triangles[15] = DirectX::XMFLOAT4(-size, height, size, 0.0f);
	triangleBufferData.triangles[16] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[17] = DirectX::XMFLOAT4(size, height, size, 0.0f);

	//z- wall
	triangleBufferData.triangles[18] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[19] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.triangles[20] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);

	triangleBufferData.triangles[21] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.triangles[22] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.triangles[23] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);

	//x+ wall
	triangleBufferData.triangles[24] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[25] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.triangles[26] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	triangleBufferData.triangles[27] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.triangles[28] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.triangles[29] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	//x- wall
	triangleBufferData.triangles[30] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.triangles[31] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[32] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);

	triangleBufferData.triangles[33] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.triangles[34] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.triangles[35] = DirectX::XMFLOAT4(-size, height, size, 0.0f);

	//////////////////////////////////////////////////
	//Spheres
	//////////////////////////////////////////////////
	sphereBuffer = CreateBuffer(sizeof(float) * 4 * 64 + sizeof(int) * 4, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &sphereBufferData);
	if(sphereBuffer == nullptr)
		return false;

	//////////////////////////////////////////////////
	//Triangles
	//////////////////////////////////////////////////
	triangleBuffer = CreateBuffer(sizeof(float) * 4 * 64 * 3 + sizeof(int) * 4, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &triangleBufferData);
	if(triangleBuffer == nullptr)
		return false;

	return true;
}

void MulticoreWindow::InitInput()
{
	Input::Init(hWnd);
	Input::RegisterKeyCallback(std::bind(&MulticoreWindow::KeyEvent, this, std::placeholders::_1));
	Input::RegisterMouseButtonCallback(std::bind(&MulticoreWindow::MouseEvent, this, std::placeholders::_1));
	Input::RegisterCharCallback(std::bind(&MulticoreWindow::CharEvent, this, std::placeholders::_1));
	Input::RegisterScrollCallback(std::bind(&MulticoreWindow::ScrollEvent, this, std::placeholders::_1));
}

void MulticoreWindow::InitConsole()
{
	auto style = std::shared_ptr<GUIStyle>(console.GenerateDoomStyle(&contentManager));
	auto background = std::unique_ptr<GUIBackground>(console.GenerateDoomStyleBackground(&contentManager));
	auto backgroundStyle = std::shared_ptr<GUIStyle>(console.GenerateDoomStyleBackgroundStyle(&contentManager));

	console.Init(Rect(0.0f, 0.0f, 1280.0f, 300.0f), style, background, backgroundStyle);
	console.Autoexec();

	guiManager.AddContainer(&console);

	Logger::SetCallOnLog(std::bind(&Console::AddText, &console, std::placeholders::_1));
}

COMUniquePtr<ID3D11Buffer> MulticoreWindow::CreateBuffer(const std::vector<BUFFER_DATA_TYPES>& bufferDataTypes, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
{
	unsigned int size = 0;

	for(BUFFER_DATA_TYPES dataType : bufferDataTypes)
	{
		switch(dataType)
		{
			case BUFFER_DATA_TYPES::MAT4X4:
				size += sizeof(float) * 16;
				break;
			case BUFFER_DATA_TYPES::INT:
				size += sizeof(int);
				break;
			case BUFFER_DATA_TYPES::FLOAT1:
				size += sizeof(float);
				break;
			case BUFFER_DATA_TYPES::FLOAT2:
				size += sizeof(float) * 2;
				break;
			case BUFFER_DATA_TYPES::FLOAT3:
				size += sizeof(float) * 3;
				break;
			case BUFFER_DATA_TYPES::FLOAT4:
				size += sizeof(float) * 4;
				break;
			default:
				"Couldn't find buffer data type";
				break;

		}
	}

	D3D11_BUFFER_DESC viewProjBufferDesc;
	viewProjBufferDesc.ByteWidth = size;
	viewProjBufferDesc.Usage = usage;
	viewProjBufferDesc.BindFlags = bindFlags;
	viewProjBufferDesc.CPUAccessFlags = cpuAccess;
	viewProjBufferDesc.MiscFlags = 0;
	viewProjBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = initialData;

	COMUniquePtr<ID3D11Buffer> returnBuffer = nullptr;
	ID3D11Buffer* returnBufferDumb = nullptr;

	HRESULT hRes = device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &returnBufferDumb);
	returnBuffer.reset(returnBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create buffer with size " + std::to_string(size));
		return nullptr;
	}

	return std::move(returnBuffer);
}

COMUniquePtr<ID3D11Buffer> MulticoreWindow::CreateBuffer(UINT size, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
{
	D3D11_BUFFER_DESC viewProjBufferDesc;
	viewProjBufferDesc.ByteWidth = size;
	viewProjBufferDesc.Usage = usage;
	viewProjBufferDesc.BindFlags = bindFlags;
	viewProjBufferDesc.CPUAccessFlags = cpuAccess;
	viewProjBufferDesc.MiscFlags = 0;
	viewProjBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = initialData;

	COMUniquePtr<ID3D11Buffer> returnBuffer = nullptr;
	ID3D11Buffer* returnBufferDumb = nullptr;

	HRESULT hRes = device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &returnBufferDumb);
	returnBuffer.reset(returnBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create buffer with size " + std::to_string(size));
		return nullptr;
	}

	return std::move(returnBuffer);
}

COMUniquePtr<ID3D11Buffer> MulticoreWindow::CreateBuffer(BUFFER_DATA_TYPES bufferDataType, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
{
	std::vector<BUFFER_DATA_TYPES> bufferDataTypes = { bufferDataType };

	return std::move(CreateBuffer(bufferDataTypes, usage, bindFlags, cpuAccess));
}

bool MulticoreWindow::CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;

	COMUniquePtr<ID3D11Texture2D> texture(nullptr);

	ID3D11Texture2D* textureDumb = nullptr;
	texture.reset(textureDumb);
	HRESULT hRes = device->CreateTexture2D(&desc, nullptr, &textureDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
		return false;
	}

	ID3D11UnorderedAccessView* uavDumb;
	hRes = device->CreateUnorderedAccessView(textureDumb, nullptr, &uavDumb);
	uav.reset(uavDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create UAV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
		return false;
	}

	ID3D11ShaderResourceView* srvDumb;
	hRes = device->CreateShaderResourceView(textureDumb, nullptr, &srvDumb);
	srv.reset(srvDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create SRV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
		return false;
	}

	return true;
}

bool MulticoreWindow::GenerateCubePrimitive(std::vector<unsigned int> &indexData, ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer)
{
	std::vector<Vertex> vertexData;
	//Front face
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	//Back face
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	//Left face
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	//Right face
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	//Top face
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, 5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	//Bottom face
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, 5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	vertexData.emplace_back(DirectX::XMFLOAT3(-5.0f, -5.0f, -5.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	for(int i = 0; i < vertexData.size(); i += 4)
	{
		indexData.push_back(i);
		indexData.push_back(i + 1);
		indexData.push_back(i + 2);

		indexData.push_back(i + 2);
		indexData.push_back(i + 1);
		indexData.push_back(i + 3);
	}

	D3D11_SUBRESOURCE_DATA vertexDataDesc;
	ZeroMemory(&vertexDataDesc, sizeof(vertexDataDesc));
	vertexDataDesc.pSysMem = &vertexData[0];

	D3D11_SUBRESOURCE_DATA indexDataDesc;
	ZeroMemory(&indexDataDesc, sizeof(indexDataDesc));
	indexDataDesc.pSysMem = &indexData[0];

	D3D11_BUFFER_DESC vertexDesc;
	ZeroMemory(&vertexDesc, sizeof(vertexDesc));
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.ByteWidth = sizeof(Vertex) * vertexData.size();
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_BUFFER_DESC indexDesc;
	ZeroMemory(&indexDesc, sizeof(indexDesc));
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.ByteWidth = sizeof(unsigned int) * indexData.size();
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	HRESULT hRes = device->CreateBuffer(&vertexDesc, &vertexDataDesc, vertexBuffer);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create vertex buffer");
		return false;
	}

	hRes = device->CreateBuffer(&indexDesc, &indexDataDesc, indexBuffer);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create index buffer");
		return false;
	}

	return true;
}

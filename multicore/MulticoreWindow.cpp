#include "MulticoreWindow.h"

#include <thread>
#include <vector>
#include <functional>
#include <cmath>

#include <DXLib/Logger.h>
#include <DXLib/input.h>
#include <DXLib/States.h>
#include <DXLib/OBJFile.h>

#include <DXConsole/console.h>
#include <DXConsole/commandGetSet.h>
#include <DXConsole/commandCallMethod.h>

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, billboardSamplerState(nullptr)
	, backbufferUAV(nullptr)
	, vertexBuffer(nullptr)
	, indexBuffer(nullptr)
	, outputColorUAV { nullptr, nullptr }
	, outputColorSRV{ nullptr, nullptr }
	, rayNormalUAV(nullptr)
	, rayNormalSRV(nullptr)
	, rayColorUAV(nullptr)
	, rayDirectionUAV{ nullptr, nullptr }
	, rayPositionUAV{ nullptr, nullptr }
	, rayColorSRV(nullptr)
	, rayDirectionSRV{ nullptr, nullptr }
	, rayPositionSRV{ nullptr, nullptr }
	, billboardBlendState(nullptr)
	, bulbTexture(nullptr)
	, primaryRayGenerator("main", "cs_5_0")
	, traceShader("main", "cs_5_0")
	, intersectionShader("main", "cs_5_0")
	, compositShader("main", "cs_5_0")
	, bulbVertexShader("main", "vs_5_0")
	, bulbPixelShader("main", "ps_5_0")
	, drawConsole(false)
{
}

MulticoreWindow::~MulticoreWindow()
{}

bool MulticoreWindow::Init()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	dispatchX = width / 32;
	dispatchY = height / 16;

	//Content manager
	contentManager.Init(device.get());
	calibri16 = contentManager.Load<CharacterSet>("calibri16");

	//Sprite renderer
	spriteRenderer.Init(device.get(), deviceContext.get(), &contentManager, width, height);

	InitConsole();
	InitInput();

	rayBounces = 1;
	auto rayBouncesCommand = new CommandGetSet<int>("RayBounces", &rayBounces);
	if(!console.AddCommand(rayBouncesCommand))
		delete rayBouncesCommand;

	auto resetCamera = new CommandCallMethod("ResetCamera", std::bind(&MulticoreWindow::ResetCamera, this, std::placeholders::_1));
	auto pauseCamera = new CommandCallMethod("PauseCamera", std::bind(&MulticoreWindow::PauseCamera, this, std::placeholders::_1));
	auto startCamera = new CommandCallMethod("StartCamera", std::bind(&MulticoreWindow::StartCamera, this, std::placeholders::_1));
	auto addCameraSnapshot = new CommandCallMethod("AddCameraSnapshot", std::bind(&MulticoreWindow::AddCameraSnapshot, this, std::placeholders::_1));

	console.AddCommand(resetCamera);
	console.AddCommand(pauseCamera);
	console.AddCommand(startCamera);
	console.AddCommand(addCameraSnapshot);

	InitOBJ();

	if(!InitSRVs())
		return false;
	if(!InitRoom())
		return false;
	if(!InitPointLights())
		return false;

	LogErrorReturnFalse(objBuffer.Create<TriangleBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &triangleBufferData), "Couldn't create triangle buffer: ");
	if(!InitRaytraceShaders())
		return false;


	if(!InitBulb())
		return false;
	if(!InitGraphs())
		return false;
	
	currentCamera = &fpsCamera;

	//Etc
	fpsCamera.InitFovHorizontal(DirectX::XMFLOAT3(3.0f, 3.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 100.0f);
	cinematicCamera.InitFovHorizontal(DirectX::XMFLOAT3(0.0f, 3.0f, -7.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 100.0f);

	cameraLookAt = DirectX::XMLoadFloat3(pointLightBufferData.lights[0]);
	cinematicCamera.LookAtSlerp(&cameraLookAt, 5000.0f);
	cinematicCamera.SetLoop(true);
	cinematicCamera.Pause();

	POINT midPoint;
	midPoint.x = 640;
	midPoint.y = 360;

	ClientToScreen(hWnd, &midPoint);
	SetCursorPos(midPoint.x, midPoint.y);

	console.Autoexec();

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
float otherSinVal = 0.0f;

void MulticoreWindow::Update(std::chrono::nanoseconds delta)
{
	float deltaMS = delta.count() * 1e-6f;

	//cinematicCamera.Update(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(500)));
	//cameraLookAt = DirectX::XMLoadFloat3(pointLightBufferData.lights[lightFocus]);

	//cinematicCamera.Update(delta);
	//cinematicCamera.LookAt(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
	//cinematicCamera.LookAt(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
	//cinematicCamera.SetPosition(DirectX::XMFLOAT3(std::cosf(sinVal * 0.5f), 3.0f, std::sinf(sinVal * 0.5f)));

	sinVal += deltaMS * lightVerticalSpeed;
	otherSinVal += deltaMS * lightHorizontalSpeed;

	if(!drawConsole)
	{
		POINT cursorPosition;
		GetCursorPos(&cursorPosition);
		ScreenToClient(hWnd, &cursorPosition);

		POINT midPoint;
		midPoint.x = 640;
		midPoint.y = 360;

		float xDelta = static_cast<float>(cursorPosition.x - midPoint.x);
		float yDelta = static_cast<float>(cursorPosition.y - midPoint.y);

		float sensitivity = 0.001f;

		fpsCamera.Rotate(DirectX::XMFLOAT2(xDelta * sensitivity, yDelta * sensitivity));
		ClientToScreen(hWnd, &midPoint);
		SetCursorPos(midPoint.x, midPoint.y);

		float cameraSpeed = 0.005f * deltaMS;

		if(keyMap.count('W'))
			fpsCamera.MoveFoward(cameraSpeed);
		else if(keyMap.count('S'))
			fpsCamera.MoveFoward(-cameraSpeed);

		if(keyMap.count('A'))
			fpsCamera.MoveRight(-cameraSpeed);
		else if(keyMap.count('D'))
			fpsCamera.MoveRight(cameraSpeed);

		if(keyMap.count(VK_SPACE))
			fpsCamera.MoveUp(cameraSpeed);
		else if(keyMap.count(VK_CONTROL))
			fpsCamera.MoveUp(-cameraSpeed);
	}

	guiManager.Update(delta);
}

void MulticoreWindow::Draw()
{
	float colors[] = { 44.0f / 255.0f, 87.0f / 255.0f, 120.0f / 255.0f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRenderTarget.get(), colors);

	DrawUpdateMVP();
	DrawUpdatePointlights();

	//////////////////////////////////////////////////
	//Rays
	//////////////////////////////////////////////////
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

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
			gpuGraph.AddValueToTrack(pair.first, static_cast<float>(pair.second));
	}

	gpuGraph.AddValueToTrack("Trace", traceTime);
	gpuGraph.AddValueToTrack("Shade", shadeTime);

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
	DrawBulbs();

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

		if(keyCode.key == VK_F1)
			currentCamera = &fpsCamera;
		else if(keyCode.key == VK_F2)
			currentCamera = &cinematicCamera;

		if(keyCode.key == VK_UP)
		{
			lightFocus++;
			cinematicCamera.LookAtSlerp(&cameraLookAt, 1000.0f);
		}
		else if(keyCode.key == VK_DOWN)
		{
			lightFocus--;
			cinematicCamera.LookAtSlerp(&cameraLookAt, 1000.0f);
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

Argument MulticoreWindow::ResetCamera(const std::vector<Argument>& argument)
{
	cinematicCamera.Reset();

	return "";
}

Argument MulticoreWindow::PauseCamera(const std::vector<Argument>& argument)
{
	cinematicCamera.Pause();

	return "";
}

Argument MulticoreWindow::StartCamera(const std::vector<Argument>& argument)
{
	cinematicCamera.Start();

	return "";
}

Argument MulticoreWindow::AddCameraSnapshot(const std::vector<Argument>& argument)
{
	CameraKeyFrame newFrame;

	newFrame.lookMode = LOOK_MODE::NONE;
	newFrame.position = currentCamera->GetPosition();

	return "";
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

	//Color
	if(!CreateUAVSRVCombo(width, height, rayColorUAV, rayColorSRV))
		return false;

	//Output
	if(!CreateUAVSRVCombo(width, height, outputColorUAV[0], outputColorSRV[0]))
		return false;

	if(!CreateUAVSRVCombo(width, height, outputColorUAV[1], outputColorSRV[1]))
		return false;

	return true;
}

bool MulticoreWindow::InitRaytraceShaders()
{
	//////////////////////////////////////////////////
	//Cbuffers
	//////////////////////////////////////////////////
	LogErrorReturnFalse(viewProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXBuffer::TYPE::FLOAT4X4), "Couldn't create view proj buffer: ");
	LogErrorReturnFalse(viewProjInverseBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXBuffer::TYPE::FLOAT4X4), "Couldn't create view proj inverse buffer: ");

	//////////////////////////////////////////////////
	//Primary rays
	//////////////////////////////////////////////////
	ShaderResourceBinds primaryResourceBinds0;
	primaryResourceBinds0.AddResource(viewProjInverseBuffer.GetBuffer(), 0);

	primaryResourceBinds0.AddResource(rayPositionUAV[0].get(), 0);
	primaryResourceBinds0.AddResource(rayDirectionUAV[0].get(), 1);
	primaryResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	primaryResourceBinds0.AddResource(outputColorUAV[1].get(), 3);

	LogErrorReturnFalse(primaryRayGenerator.CreateFromFile("PrimaryRayGenerator.hlsl", device.get(), primaryResourceBinds0), "");

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBinds0;
	traceResourceBinds0.AddResource(sphereBuffer.GetBuffer(), 0);
	traceResourceBinds0.AddResource(objBuffer.GetBuffer(), 1);

	traceResourceBinds0.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds0.AddResource(rayColorUAV.get(), 3);

	traceResourceBinds0.AddResource(rayPositionSRV[0].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionSRV[0].get(), 1);

	ShaderResourceBinds traceResourceBinds1;
	traceResourceBinds1.AddResource(sphereBuffer.GetBuffer(), 0);
	traceResourceBinds1.AddResource(objBuffer.GetBuffer(), 1);

	traceResourceBinds1.AddResource(rayPositionUAV[0].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionUAV[0].get(), 1);
	traceResourceBinds1.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds1.AddResource(rayColorUAV.get(), 3);

	traceResourceBinds1.AddResource(rayPositionSRV[1].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionSRV[1].get(), 1);

	LogErrorReturnFalse(traceShader.CreateFromFile("Intersection.hlsl", device.get(), traceResourceBinds0, traceResourceBinds1), "");

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds0;
	shadeResourceBinds0.AddResource(pointLightBuffer.GetBuffer(), 0);
	shadeResourceBinds0.AddResource(sphereBuffer.GetBuffer(), 1);
	shadeResourceBinds0.AddResource(objBuffer.GetBuffer(), 2);
	shadeResourceBinds0.AddResource(pointlightAttenuationBuffer.GetBuffer(), 3);
	shadeResourceBinds0.AddResource(cameraPositionBuffer.GetBuffer(), 4);

	shadeResourceBinds0.AddResource(outputColorUAV[0].get(), 0);

	shadeResourceBinds0.AddResource(rayPositionSRV[1].get(), 0);
	shadeResourceBinds0.AddResource(rayNormalSRV.get(), 1);
	shadeResourceBinds0.AddResource(rayColorSRV.get(), 2);
	shadeResourceBinds0.AddResource(outputColorSRV[1].get(), 3);

	ShaderResourceBinds shadeResourceBinds1;
	shadeResourceBinds1.AddResource(pointLightBuffer.GetBuffer(), 0);
	shadeResourceBinds1.AddResource(sphereBuffer.GetBuffer(), 1);
	shadeResourceBinds1.AddResource(objBuffer.GetBuffer(), 2);
	shadeResourceBinds1.AddResource(pointlightAttenuationBuffer.GetBuffer(), 3);
	shadeResourceBinds1.AddResource(cameraPositionBuffer.GetBuffer(), 4);

	shadeResourceBinds1.AddResource(outputColorUAV[1].get(), 0);

	shadeResourceBinds1.AddResource(rayPositionSRV[0].get(), 0);
	shadeResourceBinds1.AddResource(rayNormalSRV.get(), 1);
	shadeResourceBinds1.AddResource(rayColorSRV.get(), 2);
	shadeResourceBinds1.AddResource(outputColorSRV[0].get(), 3);

	LogErrorReturnFalse(intersectionShader.CreateFromFile("Shading.hlsl", device.get(), shadeResourceBinds0, shadeResourceBinds1), "");

	auto reloadShadersCommand = new CommandCallMethod("ReloadShaders", std::bind(&MulticoreWindow::ReloadRaytraceShaders, this, std::placeholders::_1), false);
	if(!console.AddCommand(reloadShadersCommand))
		delete reloadShadersCommand;

	ShaderResourceBinds compositResourceBinds0; 
	compositResourceBinds0.AddResource(backbufferUAV.get(), 0);
	compositResourceBinds0.AddResource(outputColorSRV[0].get(), 0);

	ShaderResourceBinds compositResourceBinds1;
	compositResourceBinds1.AddResource(backbufferUAV.get(), 0);
	compositResourceBinds1.AddResource(outputColorSRV[1].get(), 0);

	LogErrorReturnFalse(compositShader.CreateFromFile("Composit.hlsl", device.get(), compositResourceBinds0, compositResourceBinds1), "");

	return true;
}

bool MulticoreWindow::InitBulb()
{
	LogErrorReturnFalse(bulbPixelShader.CreateFromFile("BulbPixelShader.cso", device.get()), "Couldnt' load bulb shader: ");
	LogErrorReturnFalse(bulbVertexShader.CreateFromFile("BulbVertexShader.cso", device.get()), "Couldnt' load bulb shader: ");

	bulbVertexShader.SetVertexData(device.get(),
		std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4X4 }
	, std::vector<std::string> { "POSITION", "COLOR", "TEX_COORD", "WORLD_MATRIX" }
	, std::vector<bool> { false, false, false, true });

	std::vector<Vertex> bulbVertices;
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	LogErrorReturnFalse(bulbInstanceBuffer.Create<Float4x4BufferData>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, MAX_POINT_LIGHTS), "Couldn't create bulb instance buffer: ");
	LogErrorReturnFalse(bulbVertexBuffer.Create<Vertex>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), 6, &bulbVertices[0]), "Couldn't create bulb vertex buffer: ");
	LogErrorReturnFalse(bulbProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXBuffer::TYPE::FLOAT4X4), "Couldn't create bult projection matrix buffer: ");

	bulbTexture = contentManager.Load<Texture2D>("Bulb.dds");
	if(bulbTexture == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't load bulb texture");
		return false;
	}

	billboardBlendState = BlendStates::singleDefault;
	billboardSamplerState = SamplerStates::linearClamp;
	billboardRasterizerState = RasterizerStates::solid;

	return true;
}

bool MulticoreWindow::InitPointLights()
{
	LogErrorReturnFalse(cameraPositionBuffer.Create<CameraPositionBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create point light camera position buffer: ");

	LogErrorReturnFalse(pointlightAttenuationBuffer.Create<LightAttenuationBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, static_cast<void*>(&pointLightBufferData)), "Couldn't create point light buffer: ");

	pointlightAttenuationBufferData.factors[0] = 2.5f;
	pointlightAttenuationBufferData.factors[1] = 0.2f;
	pointlightAttenuationBufferData.factors[2] = 1.0f;

	pointlightAttenuationBuffer.Update(deviceContext.get(), &pointlightAttenuationBufferData);

	lightRadius = 15.0f;

	pointLightBufferData.lightCount = 5;
	for(int i = 0; i < MAX_POINT_LIGHTS; i++)
		pointLightBufferData.lights[i].w = lightRadius;

	LogErrorReturnFalse(pointLightBuffer.Create<PointlightBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, static_cast<void*>(&pointLightBufferData)), "Couldn't create point light buffer: ");

	lightRotationRadius = 5.0f;
	lightMinHeight = 1.0f;
	lightMaxHeight = 9.5f;

	lightVerticalSpeed = 0.0005f;
	lightHorizontalSpeed = 0.0005f;

	lightSinValMult = 2.0f;
	lightOtherSinValMult = 1.0f;

	auto lightRotationRadiusCommand = new CommandGetSet<float>("lightRotationRadius", &lightRotationRadius);
	auto lightRadiusCommand = new CommandGetSet<float>("lightRadius", &lightRadius);
	auto lightMinHeightCommand = new CommandGetSet<float>("lightMinHeight", &lightMinHeight);
	auto lightMaxHeightCommand = new CommandGetSet<float>("lightMaxHeight", &lightMaxHeight);
	auto lightVerticalSpeedCommand = new CommandGetSet<float>("lightVerticalSpeed", &lightVerticalSpeed);
	auto lightHorizontalSpeedCommand = new CommandGetSet<float>("lightHorizontalSpeed", &lightHorizontalSpeed);
	auto lightSinValMultCommand = new CommandGetSet<float>("lightSinValMult", &lightSinValMult);
	auto lightOtherSinValMultCommand = new CommandGetSet<float>("lightOtherSinValMult", &lightOtherSinValMult);

	if(!console.AddCommand(lightRotationRadiusCommand))
		delete lightRotationRadiusCommand;
	if(!console.AddCommand(lightRadiusCommand))
		delete lightRadiusCommand;
	if(!console.AddCommand(lightMinHeightCommand))
		delete lightMinHeightCommand;
	if(!console.AddCommand(lightMaxHeightCommand))
		delete lightMaxHeightCommand;
	if(!console.AddCommand(lightVerticalSpeedCommand))
		delete lightVerticalSpeedCommand;
	if(!console.AddCommand(lightHorizontalSpeedCommand))
		delete lightHorizontalSpeedCommand;
	if(!console.AddCommand(lightSinValMultCommand))
		delete lightSinValMultCommand;
	if(!console.AddCommand(lightOtherSinValMultCommand))
		delete lightOtherSinValMultCommand;

	auto setNumberOfLights = new CommandCallMethod("SetNumberOfLights", std::bind(&MulticoreWindow::SetNumberOfLights, this, std::placeholders::_1), false);
	if(!console.AddCommand(setNumberOfLights))
		delete setNumberOfLights;
	auto setLightAttenuationFactors = new CommandCallMethod("SetLightAttenuationFactors", std::bind(&MulticoreWindow::SetLightAttenuationFactors, this, std::placeholders::_1), false);
	if(!console.AddCommand(setLightAttenuationFactors))
		delete setLightAttenuationFactors;

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

	std::string errorString = cpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(0, 720 - 128), DirectX::XMINT2(256, 128), 30.0f, 1, width, height);
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

	std::vector<Track> gpuTracks{ Track(gpuMSAverage, 1.0f) };

	std::vector<std::string> timerQueries{ "Primary" }; // , "Trace", "Shade" };

	for(int i = 0; i < MAX_BOUNCES; i++)
	{
		timerQueries.emplace_back("Trace" + std::to_string(i));
		timerQueries.emplace_back("Shade" + std::to_string(i));
	}

	if(!d3d11Timer.Init(device.get(), deviceContext.get(), timerQueries))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize d3d11Timer");
		return false;
	}

	std::vector<std::string> gpuTrackNames{ "Primary", "Trace", "Shade" };

	errorString = gpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(256 + cpuGraph.GetBackgroundWidth(), 720 - 128), DirectX::XMINT2(256, 128), 100.0f, 1, width, height);
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

	return true;
}

bool MulticoreWindow::InitRoom()
{
	SphereBufferData sphereBufferData;

	float rotationIncrease = DirectX::XM_2PI * 0.1f;
	float heightIncrease = 0.2f;

	float rotationValue = 0.0f;
	float heightValue = 0.0f;

	float radius = 4.0f;

	sphereBufferData.sphereCount = 0;

	for(int i = 0; i < 35; ++i)
	{
		sphereBufferData.spheres[i] = DirectX::XMFLOAT4(std::cosf(rotationValue) * radius, heightValue, std::sinf(rotationValue) * radius, 0.5f);

		rotationValue += rotationIncrease;
		heightValue += heightIncrease;

		++sphereBufferData.sphereCount;
	}

	for(int i = 0; i < sphereBufferData.sphereCount; ++i)
		sphereBufferData.colors[i] = DirectX::XMFLOAT4(rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), 1.0f);

	//triangleBufferData.triangleCount = 0;

	triangleBufferData.colors[triangleBufferData.triangleCount] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 1] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);

	triangleBufferData.colors[triangleBufferData.triangleCount + 2] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 3] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	triangleBufferData.colors[triangleBufferData.triangleCount + 4] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 5] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

	triangleBufferData.colors[triangleBufferData.triangleCount + 6] = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 7] = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);

	triangleBufferData.colors[triangleBufferData.triangleCount + 8] = DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 9] = DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
	
	//triangleBufferData.colors[triangleBufferData.triangleCount + 10] = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	//triangleBufferData.colors[triangleBufferData.triangleCount + 11] = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);

	/*triangleBufferData.colors[triangleBufferData.triangleCount + 12] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	triangleBufferData.colors[triangleBufferData.triangleCount + 13] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);*/

	float height = 10.0f;
	float size = 12.0f;

	int vertexCount = swordOBJ->GetMeshes().begin()->vertices.size();
	//int vertexCount = 0;

	//Floor
	triangleBufferData.vertices[vertexCount] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 1] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 2] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	triangleBufferData.vertices[vertexCount + 3] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 4] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 5] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	//Ceiling
	triangleBufferData.vertices[vertexCount + 6] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 7] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 8] = DirectX::XMFLOAT4(size, height, -size, 0.0f);

	triangleBufferData.vertices[vertexCount + 9] = DirectX::XMFLOAT4(-size, height, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 10] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 11] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);

	//z+ wall
	triangleBufferData.vertices[vertexCount + 12] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 13] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 14] = DirectX::XMFLOAT4(-size, height, size, 0.0f);

	triangleBufferData.vertices[vertexCount + 15] = DirectX::XMFLOAT4(-size, height, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 16] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 17] = DirectX::XMFLOAT4(size, height, size, 0.0f);

	//z- wall
	triangleBufferData.vertices[vertexCount + 18] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 19] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 20] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);

	triangleBufferData.vertices[vertexCount + 21] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 22] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 23] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);

	//x+ wall
	triangleBufferData.vertices[vertexCount + 24] = DirectX::XMFLOAT4(size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 25] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 26] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

	triangleBufferData.vertices[vertexCount + 27] = DirectX::XMFLOAT4(size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 28] = DirectX::XMFLOAT4(size, height, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 29] = DirectX::XMFLOAT4(size, 0.0f, size, 0.0f);

/*
	//half (z+) x- wall
	triangleBufferData.vertices[vertexCount + 30] = DirectX::XMFLOAT4(-size, 0.0f, 0.0f, 0.0f);
	triangleBufferData.vertices[vertexCount + 31] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 32] = DirectX::XMFLOAT4(-size, height, 0.0f, 0.0f);

	triangleBufferData.vertices[vertexCount + 33] = DirectX::XMFLOAT4(-size, height, 0.0f, 0.0f);
	triangleBufferData.vertices[vertexCount + 34] = DirectX::XMFLOAT4(-size, 0.0f, size, 0.0f);
	triangleBufferData.vertices[vertexCount + 35] = DirectX::XMFLOAT4(-size, height, size, 0.0f);

	//half (z-) x- wall
	triangleBufferData.vertices[vertexCount + 36] = DirectX::XMFLOAT4(-size, 0.0f, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 37] = DirectX::XMFLOAT4(-size, 0.0f, 0.0f, 0.0f);
	triangleBufferData.vertices[vertexCount + 38] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);

	triangleBufferData.vertices[vertexCount + 39] = DirectX::XMFLOAT4(-size, height, -size, 0.0f);
	triangleBufferData.vertices[vertexCount + 40] = DirectX::XMFLOAT4(-size, 0.0f, 0.0f, 0.0f);
	triangleBufferData.vertices[vertexCount + 41] = DirectX::XMFLOAT4(-size, height, 0.0f, 0.0f);*/

	for(int i = 0; i < 10; i++)
	{
		triangleBufferData.triangleIndicies[triangleBufferData.triangleCount] = DirectX::XMINT4(vertexCount + i * 3, vertexCount + i * 3 + 1, vertexCount + i * 3 + 2, 0);
		triangleBufferData.triangleCount++;
	}

	//////////////////////////////////////////////////
	//Buffers
	//////////////////////////////////////////////////
	LogErrorReturnFalse(sphereBuffer.Create<SphereBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &sphereBufferData), "Couldn't create sphere buffer: ");
	//LogErrorReturnFalse(triangleBuffer.Create<TriangleBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &triangleBufferData), "Couldn't create triangle buffer: ");

	return true;
}

bool MulticoreWindow::InitOBJ()
{
	swordOBJ = contentManager.Load<OBJFile>("sword.obj");
	if(swordOBJ == nullptr)
		return false;

	Mesh swordMesh = swordOBJ->GetMeshes().front();
	if(swordMesh.vertices.size() > MAX_VERTICES)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "OBJ mesh contains " + std::to_string(swordMesh.vertices.size()) + " vertices. This is more than " + std::to_string(MAX_VERTICES) + " (which is MAX_VERTICES) vertices");
		return false;
	}
	if(swordMesh.vertices.size() > MAX_INDICIES)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "OBJ mesh contains " + std::to_string(swordMesh.indicies.size()) + " indicies. This is more than " + std::to_string(MAX_INDICIES) + " (which is MAX_INDICIES indicies");
		return false;
	}

	for(int i = 0, end = static_cast<int>(swordMesh.vertices.size()); i < end; ++i)
	{
		triangleBufferData.vertices[i].x = swordMesh.vertices[i].position.x;
		triangleBufferData.vertices[i].y = swordMesh.vertices[i].position.y + 3.0f;
		triangleBufferData.vertices[i].z = swordMesh.vertices[i].position.z;
		triangleBufferData.vertices[i].w = 0.0f;
	}

	for(int i = 0, end = static_cast<int>(swordMesh.indicies.size()) / 3; i < end; ++i)
	{
		triangleBufferData.triangleIndicies[i].x = swordMesh.indicies[i * 3];
		triangleBufferData.triangleIndicies[i].y = swordMesh.indicies[i * 3 + 1];
		triangleBufferData.triangleIndicies[i].z = swordMesh.indicies[i * 3 + 2];
		triangleBufferData.triangleIndicies[i].w = 0.0f;
	}

	triangleBufferData.triangleCount = swordMesh.indicies.size() / 3;
	if(swordMesh.indicies.size() % 3 != 0)
		Logger::LogLine(LOG_TYPE::WARNING, "OBJ indicies count isn't divisibly by 3");

	for(int i = 0; i < triangleBufferData.triangleCount; ++i)
		triangleBufferData.colors[i] = DirectX::XMFLOAT4(rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), 0.05f);

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

	console.Init(Rect(0.0f, 0.0f, static_cast<float>(this->width), 300.0f), style, background, backgroundStyle);

	guiManager.AddContainer(&console);

	Logger::SetCallOnLog(std::bind(&Console::AddText, &console, std::placeholders::_1));
}

void MulticoreWindow::DrawUpdatePointlights()
{
	//First light is special and extra cool
	pointLightBufferData.lights[0].x = 0.0f;
	pointLightBufferData.lights[0].y = ((1.0f + std::sinf(sinVal)) * 0.5f) * (9.8f - 3.5f) + 3.5f;
	pointLightBufferData.lights[0].z = 0.0f;
	pointLightBufferData.lights[0].w = lightRadius;

	float angleIncrease = DirectX::XM_2PI / (pointLightBufferData.lightCount - 1) * lightOtherSinValMult;
	float heightIncrease = DirectX::XM_2PI / (pointLightBufferData.lightCount - 1) * lightSinValMult;

	for(int i = 1; i < pointLightBufferData.lightCount; i++)
	{
		pointLightBufferData.lights[i].x = std::cosf(otherSinVal + angleIncrease * i) * lightRotationRadius;
		pointLightBufferData.lights[i].y = ((1.0f + std::sinf(sinVal + heightIncrease * i)) * 0.5f) * (lightMaxHeight - lightMinHeight) + lightMinHeight;
		pointLightBufferData.lights[i].z = std::sinf(otherSinVal + angleIncrease * i) * lightRotationRadius;
		pointLightBufferData.lights[i].w = lightRadius;
	}

	pointLightBuffer.Update(deviceContext.get(), &pointLightBufferData);
}

void MulticoreWindow::DrawUpdateMVP()
{
	DirectX::XMFLOAT4X4 viewMatrix = currentCamera->GetViewMatrix();
	DirectX::XMFLOAT4X4 projectionMatrix = currentCamera->GetProjectionMatrix();
	DirectX::XMFLOAT4X4 viewProjectionMatrix;

	DirectX::XMMATRIX xmViewMatrix = DirectX::XMLoadFloat4x4(&viewMatrix);
	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMLoadFloat4x4(&projectionMatrix);

	DirectX::XMMATRIX xmViewProjMatrix = DirectX::XMMatrixMultiplyTranspose(xmViewMatrix, xmProjectionMatrix);

	Float4x4BufferData viewProjBuffer;

	DirectX::XMStoreFloat4x4(&viewProjectionMatrix, xmViewProjMatrix);
	DirectX::XMStoreFloat4x4(&viewProjBuffer.matrix, DirectX::XMMatrixInverse(nullptr, xmViewProjMatrix));

	viewProjInverseBuffer.Update(deviceContext.get(), &viewProjBuffer);

	//Bulb viewProjection
	bulbProjMatrixBuffer.Update(deviceContext.get(), &xmViewProjMatrix);

	//Camera position for shading
	DirectX::XMFLOAT3 cameraPosition = currentCamera->GetPosition();

	cameraPositionBuffer.Update(deviceContext.get(), &cameraPosition);
}

void MulticoreWindow::DrawRayPrimary()
{
	primaryRayGenerator.Bind(deviceContext.get());
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	primaryRayGenerator.Unbind(deviceContext.get());
}

void MulticoreWindow::DrawRayIntersection(int config)
{
	traceShader.Bind(deviceContext.get(), config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	traceShader.Unbind(deviceContext.get());
}

void MulticoreWindow::DrawRayShading(int config)
{
	intersectionShader.Bind(deviceContext.get(), config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	intersectionShader.Unbind(deviceContext.get());
}

void MulticoreWindow::DrawComposit(int config)
{
	compositShader.Bind(deviceContext.get(), config);
	deviceContext->Dispatch(dispatchX, dispatchY, 1);
	compositShader.Unbind(deviceContext.get());
}

void MulticoreWindow::DrawBulbs()
{
	for(int i = 0; i < pointLightBufferData.lightCount; i++)
	{
		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixTranslation(pointLightBufferData.lights[i].x, pointLightBufferData.lights[i].y, pointLightBufferData.lights[i].z);
		DirectX::XMStoreFloat4x4(&bulbInstanceData[i].matrix, worldMatrix);
	}

	bulbInstanceBuffer.Update(deviceContext.get(), &bulbInstanceData);

	ID3D11Buffer* bulbBuffers[] = { bulbProjMatrixBuffer.GetBuffer() };
	deviceContext->VSSetConstantBuffers(0, 1, bulbBuffers);

	UINT bulbStrides[] = { sizeof(Vertex), sizeof(Float4x4BufferData) };
	UINT bulbOffsets[] = { 0, 0 };

	ID3D11Buffer* bulbVertexBuffers[] = { bulbVertexBuffer.GetBuffer(), bulbInstanceBuffer.GetBuffer() };
	deviceContext->IASetVertexBuffers(0, 2, bulbVertexBuffers, bulbStrides, bulbOffsets);

	ID3D11ShaderResourceView* bulbTextureDumb = bulbTexture->GetTextureResourceView();
	deviceContext->PSSetShaderResources(0, 1, &bulbTextureDumb);

	deviceContext->PSSetSamplers(0, 1, &billboardSamplerState);

	float blendFac[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	deviceContext->OMSetBlendState(billboardBlendState, blendFac, 0xFFFFFFFF);
	deviceContext->RSSetState(billboardRasterizerState);

	bulbVertexShader.Bind(deviceContext.get());
	bulbPixelShader.Bind(deviceContext.get());
	deviceContext->DrawInstanced(6, pointLightBufferData.lightCount, 0, 0);
	bulbVertexShader.Unbind(deviceContext.get());
	bulbPixelShader.Unbind(deviceContext.get());
}

Argument MulticoreWindow::SetNumberOfLights(const std::vector<Argument>& argument)
{
	if(argument.size() != 1)
		return "Expected 1 argument";

	if(argument.front().values.size() != 1)
		return "Expected 1 argument";

	int newCount = 0;

	try
	{
		newCount = std::stoi(argument.front().values.front());
	}
	catch(std::invalid_argument&)
	{
		return "Couldn't convert argument to an int";
	}
	catch(std::out_of_range&)
	{
		return "Expected 0-10 lights";
	}

	if(newCount > 10 || newCount < 0)
		return "Expected 0-10 lights";

	for(int i = pointLightBufferData.lightCount, end = newCount; i < newCount; ++i)
		pointLightBufferData.lights[i].w = lightRadius;

	pointLightBufferData.lightCount = newCount;

	return "lightCount set";
}

Argument MulticoreWindow::SetLightAttenuationFactors(const std::vector<Argument>& argument)
{
	if(argument.size() > 3)
		return "Expected 3 arguments";

	for(int i = 0; i < argument.size(); i++)
	{
		float factor;

		if(!(argument[i] >> factor))
			return "Couldn't convert second argument to float";

		pointlightAttenuationBufferData.factors[i] = factor;
	}

	pointlightAttenuationBuffer.Update(deviceContext.get(), &pointlightAttenuationBufferData);

	return "Updated attenuation factors";
}

Argument MulticoreWindow::ReloadRaytraceShaders(const std::vector<Argument>& argument)
{
	std::string errorString = primaryRayGenerator.Recreate(device.get());
	if(!errorString.empty())
		return "Couldn't reload primary ray shader: " + errorString;
	errorString = traceShader.Recreate(device.get());
	if(!errorString.empty())
		return "Couldn't reload primary ray shader: " + errorString;
	errorString = intersectionShader.Recreate(device.get());
	if(!errorString.empty())
		return "Couldn't reload primary ray shader: " + errorString;

	return "Successfully reloaded shaders";
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
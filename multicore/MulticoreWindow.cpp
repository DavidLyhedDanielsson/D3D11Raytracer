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
	, bezierPixelShader("main", "ps_5_0")
	, bezierVertexShader("main", "vs_5_0")
	, bezierHullShader("main", "hs_5_0")
	, bezierDomainShader("main", "ds_5_0")
	, drawConsole(false)
	, cinematicCameraMode(true)
	, bezierVertexCount(0)
	, swordOBJ(nullptr)
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
	auto addCameraFrame = new CommandCallMethod("AddCameraFrame", std::bind(&MulticoreWindow::AddCameraFrame, this, std::placeholders::_1));
	auto setCameraFrame = new CommandCallMethod("SetCameraFrame", std::bind(&MulticoreWindow::SetCameraFrame, this, std::placeholders::_1));
	auto removeCameraFrame = new CommandCallMethod("RemoveCameraFrame", std::bind(&MulticoreWindow::RemoveCameraFrame, this, std::placeholders::_1));
	auto printCameraFrames = new CommandCallMethod("PrintCameraFrames", std::bind(&MulticoreWindow::PrintCameraFrames, this, std::placeholders::_1));
	auto setCameraTargetSpeed = new CommandCallMethod("SetCameraTargetSpeed", std::bind(&MulticoreWindow::SetCameraTargetSpeed, this, std::placeholders::_1));

	console.AddCommand(resetCamera);
	console.AddCommand(pauseCamera);
	console.AddCommand(startCamera);
	console.AddCommand(addCameraFrame);
	console.AddCommand(setCameraFrame);
	console.AddCommand(removeCameraFrame);
	console.AddCommand(printCameraFrames);
	console.AddCommand(setCameraTargetSpeed);


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

	rayBounces = 1;
	
	if(cinematicCameraMode)
		currentCamera = &cinematicCamera;
	else
		currentCamera = &fpsCamera;

	//Etc
	fpsCamera.InitFovHorizontal(DirectX::XMFLOAT3(3.0f, 3.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 100.0f);
	cinematicCamera.InitFovHorizontal(DirectX::XMFLOAT3(0.0f, 3.0f, -7.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 100.0f);
	cinematicCamera.LookAt(DirectX::XMFLOAT3(0.0f, 3.5f, 0.0f));

	std::vector<CameraKeyFrame> cinematicKeyFrames;
	CameraKeyFrame newFrame;

	newFrame.position = DirectX::XMFLOAT3(-1.85f, 3.45f, -0.0f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(-0.0f, 3.5f, -2.5f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(2.5f, 3.45f, 0.0f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(2.5f, 4.5f, 4.2f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(-1.8f, 4.75f, 4.5f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(-4.0f, 5.0f, 2.65f);
	cinematicKeyFrames.push_back(newFrame);

	cinematicCamera.SetKeyFrames(cinematicKeyFrames);

	InitBezier();

	cinematicCamera.SetLoop(true);
	cinematicCamera.Reset();
	cinematicCamera.Start();

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

			/*if(std::chrono::duration_cast<std::chrono::seconds>(gameTimer.GetTime()).count() >= 24)
			{
				run = false;
				PostQuitMessage(0);

				gpuGraph.DumpValues("GPUGraph.txt");
			}*/
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

	POINT cursorPosition;
	GetCursorPos(&cursorPosition);
	ScreenToClient(hWnd, &cursorPosition);

	POINT midPoint;
	midPoint.x = 640;
	midPoint.y = 360;

	float xDelta = static_cast<float>(cursorPosition.x - midPoint.x);
	float yDelta = static_cast<float>(cursorPosition.y - midPoint.y);

	float sensitivity = 0.001f;

	if(cinematicCameraMode)
		cinematicCamera.Update(delta);
	else if(!drawConsole)
	{
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

	if(!drawConsole && !cinematicCameraMode)
	{
		currentCamera->Rotate(DirectX::XMFLOAT2(xDelta * sensitivity, yDelta * sensitivity));
		ClientToScreen(hWnd, &midPoint);
		SetCursorPos(midPoint.x, midPoint.y);
	}

	sinVal += deltaMS * lightVerticalSpeed;
	otherSinVal += deltaMS * lightHorizontalSpeed;

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

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST);
	DrawBezier();
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
		{
			cinematicCameraMode = false;
			currentCamera = &fpsCamera;
		}
		else if(keyCode.key == VK_F2)
		{
			cinematicCameraMode = true;
			currentCamera = &cinematicCamera;
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

Argument MulticoreWindow::AddCameraFrame(const std::vector<Argument>& argument)
{
	CameraKeyFrame newFrame;
	newFrame.position = currentCamera->GetPosition();
	
	cinematicCamera.AddFrame(9999, newFrame);

	UploadBezierFrames();

	return "Added frame: " + std::to_string(newFrame.position.x) + ", " + std::to_string(newFrame.position.y) + ", " + std::to_string(newFrame.position.z);
}

Argument MulticoreWindow::SetCameraFrame(const std::vector<Argument>& argument)
{
	if(argument.size() != 4
		&& argument.size() != 1)
		return "Expected 1 or 4 argument";

	int index;
	argument[0] >> index;

	float x;
	float y;
	float z;

	if(argument.size() == 1)
	{
		x = currentCamera->GetPosition().x;
		y = currentCamera->GetPosition().y;
		z = currentCamera->GetPosition().z;
	}
	else
	{
		argument[1] >> x;
		argument[2] >> y;
		argument[3] >> z;
	}

	CameraKeyFrame newFrame;
	newFrame.position = DirectX::XMFLOAT3(x, y, z);

	cinematicCamera.SetFrame(index, newFrame);

	UploadBezierFrames();

	return "Set frame " + std::to_string(index) + " to " + std::to_string(newFrame.position.x) + ", " + std::to_string(newFrame.position.y) + ", " + std::to_string(newFrame.position.z);
}

Argument MulticoreWindow::RemoveCameraFrame(const std::vector<Argument>& argument)
{
	if(argument.size() != 1)
		return "Expected 1 argument";

	int index;
	argument.front() >> index;

	UploadBezierFrames();

	if(cinematicCamera.RemoveFrame(index))
		return "Sucessfully removed frame";
	else
		return "Couldn't remove frame, check input";
}

Argument MulticoreWindow::PrintCameraFrames(const std::vector<Argument>& argument)
{
	auto frames = cinematicCamera.GetFrames();

	std::ofstream out("cameraFrames.txt", std::ios::trunc);

	if(!out.is_open())
		return "Couldn't open file";

	for(auto frame : frames)
		out << "DirectX::XMFLOAT3(" << frame.position.x << ", " << frame.position.y << ", " << frame.position.z << ")" << std::endl;

	return "Wrote to cameraFrames.txt";
}

Argument MulticoreWindow::SetCameraTargetSpeed(const std::vector<Argument>& argument)
{
	if(argument.size() != 1)
		return "Expected one argument";

	float newValue;
	argument.front() >> newValue;

	cinematicCamera.SetTargetSpeed(newValue);

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
	LogErrorReturnFalse(viewProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create view proj buffer: ");
	LogErrorReturnFalse(viewProjInverseBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create view proj inverse buffer: ");

	//////////////////////////////////////////////////
	//Primary rays
	//////////////////////////////////////////////////
	ShaderResourceBinds primaryResourceBinds0;
	primaryResourceBinds0.AddResource(viewProjInverseBuffer, 0);

	primaryResourceBinds0.AddResource(rayPositionUAV[0].get(), 0);
	primaryResourceBinds0.AddResource(rayDirectionUAV[0].get(), 1);
	primaryResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	primaryResourceBinds0.AddResource(outputColorUAV[1].get(), 3);

	LogErrorReturnFalse(primaryRayGenerator.CreateFromFile("PrimaryRayGenerator.hlsl", device.get(), primaryResourceBinds0), "");

	//////////////////////////////////////////////////
	//Intersection
	//////////////////////////////////////////////////
	ShaderResourceBinds traceResourceBinds0;
	//traceResourceBinds0.AddResource(sphereBuffer.GetBuffer(), 0);
	//traceResourceBinds0.AddResource(objBuffer.GetBuffer(), 1);

	traceResourceBinds0.AddResource(rayPositionUAV[1].get(), 0);
	traceResourceBinds0.AddResource(rayDirectionUAV[1].get(), 1);
	traceResourceBinds0.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds0.AddResource(rayColorUAV.get(), 3);

	traceResourceBinds0.AddResource(sphereBuffer, 0);
	//traceResourceBinds0.AddResource(triangleVertexBuffer, 1);
	//traceResourceBinds0.AddResource(triangleBuffer, 2);
	traceResourceBinds0.AddResource(rayPositionSRV[0].get(), 3);
	traceResourceBinds0.AddResource(rayDirectionSRV[0].get(), 4);

	ShaderResourceBinds traceResourceBinds1;
	//traceResourceBinds1.AddResource(sphereBuffer.GetBuffer(), 0);
	//traceResourceBinds1.AddResource(objBuffer.GetBuffer(), 1);

	traceResourceBinds1.AddResource(rayPositionUAV[0].get(), 0);
	traceResourceBinds1.AddResource(rayDirectionUAV[0].get(), 1);
	traceResourceBinds1.AddResource(rayNormalUAV.get(), 2);
	traceResourceBinds1.AddResource(rayColorUAV.get(), 3);

	traceResourceBinds1.AddResource(sphereBuffer, 0);
	//traceResourceBinds1.AddResource(triangleVertexBuffer, 1);
	//traceResourceBinds1.AddResource(triangleBuffer, 2);
	traceResourceBinds1.AddResource(rayPositionSRV[1].get(), 3);
	traceResourceBinds1.AddResource(rayDirectionSRV[1].get(), 4);

	LogErrorReturnFalse(traceShader.CreateFromFile("Intersection.hlsl", device.get(), traceResourceBinds0, traceResourceBinds1), "");

	//////////////////////////////////////////////////
	//Coloring
	//////////////////////////////////////////////////
	ShaderResourceBinds shadeResourceBinds0;
	shadeResourceBinds0.AddResource(pointLightBuffer, 0);
	shadeResourceBinds0.AddResource(pointlightAttenuationBuffer, 1);
	shadeResourceBinds0.AddResource(cameraPositionBuffer, 2);

	shadeResourceBinds0.AddResource(outputColorUAV[0].get(), 0);

	shadeResourceBinds0.AddResource(sphereBuffer, 0);
	//shadeResourceBinds0.AddResource(triangleVertexBuffer, 1);
	//shadeResourceBinds0.AddResource(triangleBuffer, 2);
	shadeResourceBinds0.AddResource(rayPositionSRV[1].get(), 3);
	shadeResourceBinds0.AddResource(rayNormalSRV.get(), 4);
	shadeResourceBinds0.AddResource(rayColorSRV.get(), 5);
	shadeResourceBinds0.AddResource(outputColorSRV[1].get(), 6);

	ShaderResourceBinds shadeResourceBinds1;
	shadeResourceBinds1.AddResource(pointLightBuffer, 0);
	shadeResourceBinds1.AddResource(pointlightAttenuationBuffer, 1);
	shadeResourceBinds1.AddResource(cameraPositionBuffer, 2);

	shadeResourceBinds1.AddResource(outputColorUAV[1].get(), 0);

	shadeResourceBinds1.AddResource(sphereBuffer, 0);
	//shadeResourceBinds1.AddResource(triangleVertexBuffer, 1);
	//shadeResourceBinds1.AddResource(triangleBuffer, 2);
	shadeResourceBinds1.AddResource(rayPositionSRV[0].get(), 3);
	shadeResourceBinds1.AddResource(rayNormalSRV.get(), 4);
	shadeResourceBinds1.AddResource(rayColorSRV.get(), 5);
	shadeResourceBinds1.AddResource(outputColorSRV[0].get(), 6);

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
	LogErrorReturnFalse(bulbViewProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create bulb projection matrix buffer: ");

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
	int cpuMSAverage = 10;
#endif
	std::vector<Track> cpuTracks{ Track(cpuMSAverage, 1.0f) };
	std::vector<std::string> cpuTrackNames{ "Update", "Draw" };

	std::string errorString = cpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(0, 720 - 128), DirectX::XMINT2(256, 128), 30.0f, 1, width, height, true);
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

	errorString = gpuGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(256 + cpuGraph.GetBackgroundWidth(), 720 - 128), DirectX::XMINT2(256, 128), 100.0f, 1, width, height, true);
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
	//LogErrorReturnFalse(objBuffer.Create<TriangleBufferData>(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &triangleBufferData), "Couldn't create triangle buffer: ");

	std::vector<Sphere> spheres;

	float rotationIncrease = DirectX::XM_2PI * 0.1f;
	float heightIncrease = 0.2f;

	float rotationValue = 0.0f;
	float heightValue = 2.0f;

	float radius = 4.0f;

	float roomHeight = 10.0f;
	float roomSize = 12.0f;

	float tableHeight = 3.0f;
	float tableSize = 2.0f;

	//spheres.emplace_back(DirectX::XMFLOAT3(0.0f, roomHeight, 0.0f), 3.0f, DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	for(int i = 0; i < 128; ++i)
	{
		Sphere newSphere;

		newSphere.position = DirectX::XMFLOAT3(std::cosf(rotationValue) * radius, heightValue, std::sinf(rotationValue) * radius);
		newSphere.radius = 0.5f;
		if(i % 3 == 0)
			newSphere.color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.8f);
		else if(i % 3 == 1)
			newSphere.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.8f);
		else if(i % 3 == 2)
			newSphere.color = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.8f);

		rotationValue += rotationIncrease;
		heightValue += heightIncrease;

		spheres.push_back(std::move(newSphere));
	}

	/*std::pair<std::vector<TriangleVertex>, std::vector<Triangle>> triangles = InitOBJ();

	//////////////////////////////////////////////////
	//Room
	//////////////////////////////////////////////////
	std::vector<DirectX::XMFLOAT3> newVertices;
	std::vector<DirectX::XMFLOAT4> newColors;

	//Floor
	newVertices.emplace_back(-roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);

	newVertices.emplace_back(-roomSize, 0.0f, roomSize);
	newVertices.emplace_back(-roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);

	//Ceiling
	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, roomHeight, roomSize);
	newVertices.emplace_back(roomSize, roomHeight, -roomSize);

	newVertices.emplace_back(-roomSize, roomHeight, roomSize);
	newVertices.emplace_back(roomSize, roomHeight, roomSize);
	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);

	//z+ wall
	newVertices.emplace_back(-roomSize, 0.0f, roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);
	newVertices.emplace_back(-roomSize, roomHeight, roomSize);

	newVertices.emplace_back(-roomSize, roomHeight, roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);
	newVertices.emplace_back(roomSize, roomHeight, roomSize);

	//z- wall
	newVertices.emplace_back(-roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, -roomSize);

	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, -roomSize);

	//x+ wall
	newVertices.emplace_back(roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);

	newVertices.emplace_back(roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(roomSize, roomHeight, roomSize);
	newVertices.emplace_back(roomSize, 0.0f, roomSize);

	//x- wall
	newVertices.emplace_back(-roomSize, 0.0f, -roomSize);
	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(-roomSize, 0.0f, roomSize);

	newVertices.emplace_back(-roomSize, roomHeight, -roomSize);
	newVertices.emplace_back(-roomSize, roomHeight, roomSize);
	newVertices.emplace_back(-roomSize, 0.0f, roomSize);


	//////////////////////////////////////////////////
	//Table
	//////////////////////////////////////////////////
	//z+ wall
	newVertices.emplace_back(-tableSize, 0.0f, tableSize);
	newVertices.emplace_back(-tableSize, tableHeight, tableSize);
	newVertices.emplace_back(tableSize, 0.0f, tableSize);

	newVertices.emplace_back(-tableSize, tableHeight, tableSize);
	newVertices.emplace_back(tableSize, tableHeight, tableSize);
	newVertices.emplace_back(tableSize, 0.0f, tableSize);

	//z- wall
	newVertices.emplace_back(-tableSize, 0.0f, -tableSize);
	newVertices.emplace_back(tableSize, 0.0f, -tableSize);
	newVertices.emplace_back(-tableSize, tableHeight, -tableSize);

	newVertices.emplace_back(-tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(tableSize, 0.0f, -tableSize);
	newVertices.emplace_back(tableSize, tableHeight, -tableSize);

	//x+ wall
	newVertices.emplace_back(tableSize, 0.0f, -tableSize);
	newVertices.emplace_back(tableSize, 0.0f, tableSize);
	newVertices.emplace_back(tableSize, tableHeight, -tableSize);

	newVertices.emplace_back(tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(tableSize, 0.0f, tableSize);
	newVertices.emplace_back(tableSize, tableHeight, tableSize);

	//x- wall
	newVertices.emplace_back(-tableSize, 0.0f, -tableSize);
	newVertices.emplace_back(-tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(-tableSize, 0.0f, tableSize);

	newVertices.emplace_back(-tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(-tableSize, tableHeight, tableSize);
	newVertices.emplace_back(-tableSize, 0.0f, tableSize);

	//Table mirror
	newVertices.emplace_back(-tableSize, tableHeight, -tableSize );
	newVertices.emplace_back(tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(tableSize, tableHeight, tableSize);

	newVertices.emplace_back(-tableSize, tableHeight, tableSize);
	newVertices.emplace_back(-tableSize, tableHeight, -tableSize);
	newVertices.emplace_back(tableSize, tableHeight, tableSize);

	//Floor
	newColors.emplace_back(0.2f, 0.3f, 0.8f, 0.0f);
	newColors.emplace_back(0.2f, 0.3f, 0.8f, 0.0f);

	//Ceiling
	newColors.emplace_back(0.5f, 0.5f, 0.6f, 0.0f);
	newColors.emplace_back(0.5f, 0.5f, 0.6f, 0.0f);

	//z+ wall
	newColors.emplace_back(0.0f, 0.0f, 0.0f, 0.8f);
	newColors.emplace_back(0.0f, 0.0f, 0.0f, 0.8f);

	//z- wall
	newColors.emplace_back(0.0f, 0.0f, 0.0f, 0.8f);
	newColors.emplace_back(0.0f, 0.0f, 0.0f, 0.8f);

	//x+ wall
	newColors.emplace_back(0.8f, 0.8f, 0.0f, 0.0f);
	newColors.emplace_back(0.8f, 0.8f, 0.0f, 0.0f);

	//x- wall
	newColors.emplace_back(0.0f, 0.8f, 0.8f, 0.0f);
	newColors.emplace_back(0.0f, 0.8f, 0.8f, 0.0f);

	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);
	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);

	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);
	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);

	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);
	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);

	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);
	newColors.emplace_back(1.0f, 1.0f, 1.0f, 0.0f);

	newColors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
	newColors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);

	int offset = triangles.first.size();

	for(int i = 0; i < newColors.size(); i++)
	{
/ *
		triangleBufferData.vertices[offset * 3 + i * 3] = newVertices[i * 3];
		triangleBufferData.vertices[offset * 3 + i * 3 + 1] = newVertices[i * 3 + 1];
		triangleBufferData.vertices[offset * 3 + i * 3 + 2] = newVertices[i * 3 + 2];

		triangleBufferData.colors[offset + i] = newColors[i];

		triangleBufferData.triangleIndicies[offset + i] = DirectX::XMINT4(offset * 3 + i * 3, offset * 3 + i * 3 + 1, offset * 3 + i * 3 + 2, 0);
		triangleBufferData.triangleCount++;* /
		for(int j = 0; j < 3; ++j)
			triangles.first.push_back(std::move(newVertices[i * 3 + j]));

		Triangle newTriangle;

		newTriangle.indicies = DirectX::XMINT3(offset + i * 3, offset + i * 3 + 1, offset + i * 3 + 2);
		newTriangle.color = newColors[i];

		triangles.second.push_back(std::move(newTriangle));
	}*/

	//////////////////////////////////////////////////
	//Buffers
	//////////////////////////////////////////////////
	LogErrorReturnFalse(sphereBuffer.Create<Sphere>(device.get(), D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), spheres.size(), &spheres[0]), "Couldn't create sphere buffer: ");
	/*LogErrorReturnFalse(triangleVertexBuffer.Create<TriangleVertex>(device.get(), D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), triangles.first.size(), &triangles.first[0]), "Couldn't create triangle vertex buffer: ");
	LogErrorReturnFalse(triangleBuffer.Create<Triangle>(device.get(), D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), triangles.second.size(), &triangles.second[0]), "Couldn't create triangle index buffer: ");*/

	return true;
}

std::pair<std::vector<TriangleVertex>, std::vector<Triangle>> MulticoreWindow::InitOBJ()
{
	std::vector<TriangleVertex> vertices;
	std::vector<Triangle> triangles;

	swordOBJ = contentManager.Load<OBJFile>("sword.obj");
	if(swordOBJ == nullptr)
		return std::make_pair(vertices, triangles);

	Mesh swordMesh = swordOBJ->GetMeshes().front();

	for(int i = 0, end = static_cast<int>(swordMesh.vertices.size()); i < end; ++i)
	{
		TriangleVertex newVertex;

		newVertex.position.x = swordMesh.vertices[i].position.x;
		newVertex.position.y = swordMesh.vertices[i].position.y + 3.5f;
		newVertex.position.z = swordMesh.vertices[i].position.z - 0.75f;

		vertices.push_back(std::move(newVertex));
	}

	for(int i = 0, end = static_cast<int>(swordMesh.indicies.size()) / 3; i < end; ++i)
	{
		/*triangleBufferData.triangleIndicies[i].x = swordMesh.indicies[i * 3];
		triangleBufferData.triangleIndicies[i].y = swordMesh.indicies[i * 3 + 1];
		triangleBufferData.triangleIndicies[i].z = swordMesh.indicies[i * 3 + 2];
		triangleBufferData.triangleIndicies[i].w = 0.0f;*/
		Triangle newTriangle;

		newTriangle.indicies.x = swordMesh.indicies[i * 3];
		newTriangle.indicies.y = swordMesh.indicies[i * 3 + 1];
		newTriangle.indicies.z = swordMesh.indicies[i * 3 + 2];

		newTriangle.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.25f);

		triangles.push_back(std::move(newTriangle));

	}

	/*triangleBufferData.triangleCount = swordMesh.indicies.size() / 3;
	if(swordMesh.indicies.size() % 3 != 0)
		Logger::LogLine(LOG_TYPE::WARNING, "OBJ indicies count isn't divisibly by 3");

	for(int i = 0; i < triangleBufferData.triangleCount; ++i)
		triangleBufferData.colors[i] = DirectX::XMFLOAT4(rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), rand() / static_cast<float>(RAND_MAX), 0.05f);*/

	//return true;
	return std::make_pair(vertices, triangles);
}

bool MulticoreWindow::InitBezier()
{
	std::vector<CameraKeyFrame> cameraKeyFrames = cinematicCamera.GetFrames();

	//Vertices
	LogErrorReturnFalse(bezierVertexBuffer.Create<BezierVertex>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, BEZIER_MAX_LINES * 2), "Couldn't create bezier vertex buffer: ");

	LogErrorReturnFalse(bezierViewProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create bezier view projection matrix buffer: ");

	UploadBezierFrames();

	//Shaders
	LogErrorReturnFalse(bezierPixelShader.CreateFromFile("BezierPixelShader.hlsl", device.get()), "Couldn't load bezier pixel shader: ");
	LogErrorReturnFalse(bezierVertexShader.CreateFromFile("BezierVertexShader.hlsl", device.get()), "Couldn't load bezier vertex shader: ");
	bezierVertexShader.SetVertexData(device.get(),
		std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT }
		, std::vector<std::string> { "BEGIN_ANCHOR", "END_ANCHOR", "BEGIN_HANDLE", "END_HANDLE", "LAMBDA" }
		, std::vector<bool> { false, false, false, false, false });


	DirectX::XMFLOAT2 tessFactors(1.0f, 1.0f);
	LogErrorReturnFalse(bezierTessFactorBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, static_cast<void*>(&tessFactors), DXConstantBuffer::TYPE::FLOAT2), "Couldn't create bezier hull shader buffer: ");

	ShaderResourceBinds hullShaderBinds;
	hullShaderBinds.AddResource(bezierTessFactorBuffer, 0);

	LogErrorReturnFalse(bezierHullShader.CreateFromFile("BezierHullShader.hlsl", device.get(), hullShaderBinds), "Couldn't load bezier hull shader: ");


	ShaderResourceBinds domainShaderBinds;

	domainShaderBinds.AddResource(bezierViewProjMatrixBuffer, 0);

	LogErrorReturnFalse(bezierDomainShader.CreateFromFile("BezierDomainShader.hlsl", device.get(), domainShaderBinds), "Couldn't load bezier domain shader: ");

	auto SetBezierTessFactorsCommand = new CommandCallMethod("SetBezierTessFactors", std::bind(&MulticoreWindow::SetBezierTessFactors, this, std::placeholders::_1), false);
	if(!console.AddCommand(SetBezierTessFactorsCommand))
		delete SetBezierTessFactorsCommand;

	return "";
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
	pointLightBufferData.lights[0].y = ((1.0f + std::sinf(sinVal)) * 0.5f) * 128 * 0.2f;
	pointLightBufferData.lights[0].z = 0.0f;
	pointLightBufferData.lights[0].w = lightRadius;

	float angleIncrease = DirectX::XM_2PI / (pointLightBufferData.lightCount - 1) * lightOtherSinValMult;
	float heightIncrease = DirectX::XM_2PI / (pointLightBufferData.lightCount - 1) * lightSinValMult;

	//float angleIncrease = DirectX::XM_2PI / pointLightBufferData.lightCount * lightOtherSinValMult;
	//float heightIncrease = DirectX::XM_2PI / pointLightBufferData.lightCount * lightSinValMult;

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
	DirectX::XMFLOAT4X4 viewProjectionMatrixInverse;

	DirectX::XMMATRIX xmViewMatrix = DirectX::XMLoadFloat4x4(&viewMatrix);
	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMLoadFloat4x4(&projectionMatrix);

	DirectX::XMMATRIX xmViewProjMatrix = DirectX::XMMatrixMultiplyTranspose(xmViewMatrix, xmProjectionMatrix);

	DirectX::XMStoreFloat4x4(&viewProjectionMatrix, xmViewProjMatrix);
	DirectX::XMStoreFloat4x4(&viewProjectionMatrixInverse, DirectX::XMMatrixInverse(nullptr, xmViewProjMatrix));

	viewProjInverseBuffer.Update(deviceContext.get(), &viewProjectionMatrixInverse);

	bulbViewProjMatrixBuffer.Update(deviceContext.get(), &xmViewProjMatrix);
	bezierViewProjMatrixBuffer.Update(deviceContext.get(), &xmViewProjMatrix);

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

	ID3D11Buffer* bulbBuffers[] = { bulbViewProjMatrixBuffer.GetBuffer() };
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

void MulticoreWindow::DrawBezier()
{
	ID3D11Buffer* bulbBuffers[] = { bulbViewProjMatrixBuffer.GetBuffer() };
	deviceContext->VSSetConstantBuffers(0, 1, bulbBuffers);

	UINT bezierStride = sizeof(BezierVertex);
	UINT bezierOffset = 0;

	ID3D11Buffer* bezierVertexBuffer = this->bezierVertexBuffer.GetBuffer();
	deviceContext->IASetVertexBuffers(0, 1, &bezierVertexBuffer, &bezierStride, &bezierOffset);

	bezierVertexShader.Bind(deviceContext.get());
	bezierPixelShader.Bind(deviceContext.get());
	bezierHullShader.Bind(deviceContext.get());
	bezierDomainShader.Bind(deviceContext.get());
	deviceContext->Draw(bezierVertexCount, 0);
	bezierVertexShader.Unbind(deviceContext.get());
	bezierPixelShader.Unbind(deviceContext.get());
	bezierHullShader.Unbind(deviceContext.get());
	bezierDomainShader.Unbind(deviceContext.get());
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

std::vector<BezierVertex> MulticoreWindow::CalcBezierVertices(const std::vector<CameraKeyFrame>& frames) const
{
	std::vector<BezierVertex> vertices;

	for(int i = 0, end = static_cast<int>(frames.size()); i < end; ++i)
	{
		CameraKeyFrame currentFrame = frames[i];
		CameraKeyFrame nextFrame;

		if(i == static_cast<int>(frames.size() - 1))
			nextFrame = frames.front();
		else
			nextFrame = frames[i + 1];

		BezierVertex vertex;
		vertex.beginAnchor = currentFrame.position;
		vertex.beginHandle = currentFrame.beginHandle;

		vertex.endAnchor = nextFrame.position;
		vertex.endHandle = nextFrame.endHandle;
		vertex.lambda = 0.0f;

		vertices.push_back(vertex);

		vertex.lambda = 1.0f;
		vertices.push_back(vertex);
	}

	return vertices;
}

void MulticoreWindow::UploadBezierFrames()
{
	auto frames = cinematicCamera.GetFrames();
	std::vector<BezierVertex> vertices = CalcBezierVertices(frames);
	
	if(vertices.size() > 0)
		bezierVertexBuffer.Update<BezierVertex>(deviceContext.get(), &vertices[0], static_cast<int>(vertices.size()));

	bezierVertexCount = vertices.size();
}

Argument MulticoreWindow::SetBezierTessFactors(const std::vector<Argument>& arguments)
{
	if(arguments.size() != 2)
		return "Expected 2 arguments";

	DirectX::XMFLOAT2 tessFactors;

	arguments[0] >> tessFactors.x;
	arguments[1] >> tessFactors.y;

	bezierTessFactorBuffer.Update(deviceContext.get(), &tessFactors);

	return "";
}

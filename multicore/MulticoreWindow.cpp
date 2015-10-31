#include "MulticoreWindow.h"

#include <thread>
#include <vector>
#include <functional>
#include <cmath>

#include <DXLib/Logger.h>
#include <DXLib/input.h>
#include <DXLib/States.h>
#include <DXLib/SamplerStates.h>

#include <DXConsole/console.h>
#include <DXConsole/commandGetSet.h>
#include <DXConsole/commandCallMethod.h>
#include <DXConsole/commandGetterSetter.h>

#include "ShaderProgram.h"
#include "ConstantBufferShaderProgram.h"
#include "StructuredBufferShaderProgram.h"
#include "AABBStructuredBufferShaderProgram.h"

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, billboardSamplerState(nullptr)
	, backBufferUAV(nullptr)
	, depthBufferUAV(nullptr)
	, billboardBlendState(nullptr)
	, bulbTexture(nullptr)
	, transferDepthVertexShader("main", "vs_5_0")
	, transferDepthPixelShader("main", "ps_5_0")
	, bulbVertexShader("main", "vs_5_0")
	, bulbPixelShader("main", "ps_5_0")
	, bezierPixelShader("main", "ps_5_0")
	, bezierVertexShader("main", "vs_5_0")
	, bezierHullShader("main", "hs_5_0")
	, bezierDomainShader("main", "ds_5_0")
	, drawConsole(false)
	, cinematicCameraMode(false)
	, bezierVertexCount(0)
	, lightSinVal(0.0f)
	, lightOtherSinVal(0.0f)
{
}

MulticoreWindow::~MulticoreWindow()
{}

bool MulticoreWindow::Init()
{
	srand(static_cast<unsigned int>(time(nullptr)));

	//Content manager
	contentManager.Init(device.get());
	calibri16 = contentManager.Load<CharacterSet>("calibri16");

	//Sprite renderer
	spriteRenderer.Init(device.get(), deviceContext.get(), &contentManager, width, height);

	InitConsole();
	InitInput();

	auto resetCamera = new CommandCallMethod("ResetCamera", std::bind(&MulticoreWindow::ResetCamera, this, std::placeholders::_1));
	auto pauseCamera = new CommandCallMethod("PauseCamera", std::bind(&MulticoreWindow::PauseCamera, this, std::placeholders::_1));
	auto startCamera = new CommandCallMethod("StartCamera", std::bind(&MulticoreWindow::StartCamera, this, std::placeholders::_1));
	auto addCameraFrame = new CommandCallMethod("AddCameraFrame", std::bind(&MulticoreWindow::AddCameraFrame, this, std::placeholders::_1));
	auto setCameraFrame = new CommandCallMethod("SetCameraFrame", std::bind(&MulticoreWindow::SetCameraFrame, this, std::placeholders::_1));
	auto removeCameraFrame = new CommandCallMethod("RemoveCameraFrame", std::bind(&MulticoreWindow::RemoveCameraFrame, this, std::placeholders::_1));
	auto printCameraFrames = new CommandCallMethod("PrintCameraFrames", std::bind(&MulticoreWindow::PrintCameraFrames, this, std::placeholders::_1));
	auto setCameraTargetSpeed = new CommandCallMethod("SetCameraTargetSpeed", std::bind(&MulticoreWindow::SetCameraTargetSpeed, this, std::placeholders::_1));
	auto reloadShaders = new CommandCallMethod("ReloadShaders", std::bind(&MulticoreWindow::ReloadShaders, this, std::placeholders::_1));

	console.AddCommand(resetCamera);
	console.AddCommand(pauseCamera);
	console.AddCommand(startCamera);
	console.AddCommand(addCameraFrame);
	console.AddCommand(setCameraFrame);
	console.AddCommand(removeCameraFrame);
	console.AddCommand(printCameraFrames);
	console.AddCommand(setCameraTargetSpeed);
	console.AddCommand(reloadShaders);

	auto rayBounces = new CommandGetterSetter<int>("rayBounces", std::bind(&MulticoreWindow::GetRayBounces, this), std::bind(&MulticoreWindow::SetRayBounces, this, std::placeholders::_1));
	auto lightAttenuation = new CommandGetterSetter<LightAttenuation>("lightAttenuationFactors", std::bind(&MulticoreWindow::GetLightAttenuationFactors, this), std::bind(&MulticoreWindow::SetLightAttenuationFactors, this, std::placeholders::_1));

	console.AddCommand(rayBounces);
	console.AddCommand(lightAttenuation);

#if USE_ALL_SHADER_PROGRAMS
	auto setShaderProgram = new CommandCallMethod("SetShaderProgram", std::bind(&MulticoreWindow::SetShaderProgram, this, std::placeholders::_1));
	console.AddCommand(setShaderProgram);

	constantBufferShaderProgram.reset(new ConstantBufferShaderProgram());
	if(!constantBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	structuredBufferShaderProgram.reset(new StructuredBufferShaderProgram());
	if(!structuredBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	aabbStructuredBufferShaderProgram.reset(new AABBStructuredBufferShaderProgram());
	if(!aabbStructuredBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	shaderPrograms.push_back(constantBufferShaderProgram.get());
	shaderPrograms.push_back(structuredBufferShaderProgram.get());
	shaderPrograms.push_back(aabbStructuredBufferShaderProgram.get());

	currentShaderProgram = aabbStructuredBufferShaderProgram.get();
#elif USE_CONSTANT_BUFFER_SHADER_PROGRAM
	constantBufferShaderProgram.reset(new ConstantBufferShaderProgram());
	if(!constantBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	currentShaderProgram = constantBufferShaderProgram.get();
#elif USE_STRUCTURED_BUFFER_SHADER_PROGRAM
	structuredBufferShaderProgram.reset(new StructuredBufferShaderProgram());
	if(!structuredBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	currentShaderProgram = structuredBufferShaderProgram.get();
#elif USE_AABBSTRUCTUREDBUFFER_SHADER_PROGRAM
	aabbStructuredBufferShaderProgram.reset(new AABBStructuredBufferShaderProgram());
	if(!aabbStructuredBufferShaderProgram->Init(device.get(), deviceContext.get(), width, height, &console, &contentManager))
		return false;

	currentShaderProgram = aabbStructuredBufferShaderProgram.get();
#endif

	if(!InitUAVs())
		return false;
	if(!InitRoom())
		return false;
	if(!InitPointLights())
		return false;

	LogErrorReturnFalse(viewProjMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create view projection matrix buffer: ");
	LogErrorReturnFalse(projMatrixBuffer.Create(device.get(), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, DXConstantBuffer::TYPE::FLOAT4X4), "Couldn't create projection matrix buffer: ");

	if(!InitFullscreenQuad())
		return false;
	if(!InitBulb())
		return false;
	if(!InitGraphs())
		return false;

#if USE_ALL_SHADER_PROGRAMS
	if(!constantBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
	if(!structuredBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
	if(!aabbStructuredBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
#elif USE_CONSTANT_BUFFER_SHADER_PROGRAM
	if(!constantBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
#elif USE_STRUCTURED_BUFFER_SHADER_PROGRAM
	if(!structuredBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
#elif USE_AABBSTRUCTUREDBUFFER_SHADER_PROGRAM
	if(!aabbStructuredBufferShaderProgram->InitBuffers(depthBufferUAV.get(), backBufferUAV.get()))
		return false;
#endif

	if(cinematicCameraMode)
		currentCamera = &cinematicCamera;
	else
		currentCamera = &fpsCamera;

	//Etc
	fpsCamera.InitFovHorizontal(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 1.0f, 1000.0f);
	cinematicCamera.InitFovHorizontal(DirectX::XMFLOAT3(0.0f, 3.0f, -7.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 100.0f);
	cinematicCamera.LookAt(DirectX::XMFLOAT3(0.0f, 3.5f, 0.0f));

	std::vector<CameraKeyFrame> cinematicKeyFrames;
	CameraKeyFrame newFrame;

	/*newFrame.position = DirectX::XMFLOAT3(-1.85f, 3.45f, -0.0f);
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
	cinematicKeyFrames.push_back(newFrame);*/

	newFrame.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	cinematicKeyFrames.push_back(newFrame);

	newFrame.position = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
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

			perFrameGraph.AddValueToTrack("Delta", gameTimer.GetDeltaMillisecondsFraction());

			//updateTimer.Reset();
			//updateTimer.Start();
			Update(gameTimer.GetDelta());
			//updateTimer.Stop();

			//drawTimer.Reset();
			//drawTimer.Start();
			Draw();
			//drawTimer.Stop();

			//cpuGraph.AddValueToTrack("Update", updateTimer.GetTimeMillisecondsFraction());
			//cpuGraph.AddValueToTrack("Draw", drawTimer.GetTimeMillisecondsFraction());

			/*if(cinematicCameraMode) //TODO!
			{
				if(std::chrono::duration_cast<std::chrono::seconds>(gameTimer.GetTime()).count() >= 24)
				{
					run = false;
					PostQuitMessage(0);

					graph.DumpValues("GPUGraph.txt");
				}
			}*/
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

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

		if(keyMap.count(VK_SHIFT))
			cameraSpeed *= 5.0f;

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

	lightSinVal += deltaMS * lightVerticalSpeed;
	lightOtherSinVal += deltaMS * lightHorizontalSpeed;

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

	std::map<std::string, double> d3d11Times = currentShaderProgram->Draw();

	float intersectionTime = 0.0f;
	float shadeTime = 0.0f;

	for(const auto& pair : d3d11Times)
	{
		if(pair.first.compare(0, 9, "Intersect") == 0)
			intersectionTime += static_cast<float>(pair.second);
		else if(pair.first.compare(0, 5, "Shade") == 0)
			shadeTime += static_cast<float>(pair.second);
		else if(pair.first.compare(0, 7, "Primary") == 0)
		{
			perFrameGraph.AddValueToTrack("Primary", static_cast<float>(pair.second));
			perSecondGraph.AddValueToTrack("Primary", static_cast<float>(pair.second));
		}
	}

	perFrameGraph.AddValueToTrack("Intersect", intersectionTime);
	perSecondGraph.AddValueToTrack("Intersect", intersectionTime);
	perFrameGraph.AddValueToTrack("Shade", shadeTime);
	perSecondGraph.AddValueToTrack("Shade", shadeTime);

	//////////////////////////////////////////////////
	//Forward rendering
	//////////////////////////////////////////////////
	
	deviceContext->ClearDepthStencilView(depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

	deviceContext->RSSetState(rasterizerState.get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->OMSetDepthStencilState(DepthStencilStates::writeOnly, 0xFFFFFFFF);

	//////////////////////////////////////////////////
	//Transfer depth buffer
	//////////////////////////////////////////////////
	renderTargets[0] = backBufferRenderTarget.get();
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	DrawTransferDepthBuffer();

	deviceContext->OMSetDepthStencilState(DepthStencilStates::readOnly, 0xFFFFFFFF);

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

	perSecondGraph.Draw(&spriteRenderer);
	perFrameGraph.Draw(&spriteRenderer);

	spriteRenderer.End();

	perSecondGraph.Draw();
	perFrameGraph.Draw();

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

Argument MulticoreWindow::ReloadShaders(const std::vector<Argument>& argument)
{
#ifdef USE_ALL_SHADER_PROGRAMS
	for(ShaderProgram* program : shaderPrograms)
	{
		std::string errorString = program->ReloadShaders();

		if(!errorString.empty())
			return errorString;
	}

	return "";
#else
	return currentShaderProgram->ReloadShaders();
#endif
}

void MulticoreWindow::SetRayBounces(int bounces)
{
#ifdef USE_ALL_SHADER_PROGRAMS
	for(ShaderProgram* program : shaderPrograms)
		program->SetRayBounces(bounces);
#else
	currentShaderProgram->SetRayBounces(bounces);
#endif
}

void MulticoreWindow::SetLightAttenuationFactors(const LightAttenuation& lightAttenuation)
{
#ifdef USE_ALL_SHADER_PROGRAMS
	for(ShaderProgram* program : shaderPrograms)
		program->SetLightAttenuationFactors(lightAttenuation);
#else
	currentShaderProgram->SetLightAttenuationFactors(lightAttenuation);
#endif
}

int MulticoreWindow::GetRayBounces() const
{
	return currentShaderProgram->GetRayBounces();
}

LightAttenuation MulticoreWindow::GetLightAttenuationFactors() const
{
	return currentShaderProgram->GetLightAttenuationFactors();
}

#if USE_ALL_SHADER_PROGRAMS
Argument MulticoreWindow::SetShaderProgram(const std::vector<Argument>& argument)
{
	if(argument.size() != 1)
		return "Expected 1 argument";

	if(argument.front().values.front() == "cbuffer")
		currentShaderProgram = constantBufferShaderProgram.get();
	else if(argument.front().values.front() == "sbuffer")
		currentShaderProgram = structuredBufferShaderProgram.get();
	else if(argument.front().values.front() == "aabbsbuffer")
		currentShaderProgram = aabbStructuredBufferShaderProgram.get();
	else
		return "Couldn't find shader program";

	return "Shader program set";
}
#endif

bool MulticoreWindow::InitUAVs()
{
	//////////////////////////////////////////////////
	//Back buffer UAV
	//////////////////////////////////////////////////
	ID3D11Texture2D* backBufferDumb = nullptr;
	HRESULT hRes = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferDumb));
	COMUniquePtr<ID3D11Texture2D> backBuffer(backBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't get back buffer from swapChain");
		return false;
	}

	ID3D11UnorderedAccessView* backBufferUAVDumb = nullptr;
	device->CreateUnorderedAccessView(backBuffer.get(), nullptr, &backBufferUAVDumb);
	backBufferUAV.reset(backBufferUAVDumb);
	if(backBufferUAV == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create back buffer UAV");
		return false;
	}

	//////////////////////////////////////////////////
	//Depth buffer UAV
	//////////////////////////////////////////////////
	if(!CreateUAVSRVCombo(width, height, depthBufferUAV, depthBufferSRV, DXGI_FORMAT_R32_FLOAT))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create back buffer UAV/SRV combo");
		return false;
	}

	return true;
}

bool MulticoreWindow::InitFullscreenQuad()
{
	LogErrorReturnFalse(transferDepthVertexShader.CreateFromFile("TransferDepthVertexShader.cso", device.get()), "Couldn't load depth vertex shader: ");

	ShaderResourceBinds pixelBinds;

	pixelBinds.AddResource(SamplerStates::pointClamp, 0);
	pixelBinds.AddResource(depthBufferSRV.get(), 0);
	pixelBinds.AddResource(projMatrixBuffer, 0);

	LogErrorReturnFalse(transferDepthPixelShader.CreateFromFile("TransferDepthPixelShader.cso", device.get(), pixelBinds), "Couldn't load depth pixel shader: ");

	transferDepthVertexShader.SetVertexData(device.get(),
		std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT2 }
		, std::vector<std::string> { "POSITION" }
		, std::vector<bool> { false });
	
	std::vector<DirectX::XMFLOAT2> quadVertices;

	quadVertices.emplace_back(-1.0f, -1.0f);
	quadVertices.emplace_back(1.0f, -1.0f);
	quadVertices.emplace_back(-1.0f, 1.0f);

	quadVertices.emplace_back(1.0f, 1.0f);
	quadVertices.emplace_back(-1.0f, 1.0f);
	quadVertices.emplace_back(1.0f, -1.0f);

	LogErrorReturnFalse(fullscreenQuadVertexBuffer.Create<DirectX::XMFLOAT2>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), 6, &quadVertices[0]), "Couldn't create bulb vertex buffer: ");

	return true;
}

bool MulticoreWindow::InitBulb()
{
	LogErrorReturnFalse(bulbPixelShader.CreateFromFile("BulbPixelShader.cso", device.get()), "Couldn't load bulb shader: ");
	LogErrorReturnFalse(bulbVertexShader.CreateFromFile("BulbVertexShader.cso", device.get()), "Couldn't load bulb shader: ");

	bulbVertexShader.SetVertexData(device.get(),
		std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT3, VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4X4 }
	, std::vector<std::string> { "POSITION", "COLOR", "TEX_COORD", "WORLD_MATRIX" }
	, std::vector<bool> { false, false, false, true });

	std::vector<BulbVertex> bulbVertices;
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	bulbVertices.emplace_back(DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	bulbVertices.emplace_back(DirectX::XMFLOAT3(0.5f, 0.5f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));

	LogErrorReturnFalse(bulbInstanceBuffer.Create<Float4x4BufferData>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, MAX_POINT_LIGHTS), "Couldn't create bulb instance buffer: ");
	LogErrorReturnFalse(bulbVertexBuffer.Create<BulbVertex>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DEFAULT, static_cast<D3D11_CPU_ACCESS_FLAG>(0), 6, &bulbVertices[0]), "Couldn't create bulb vertex buffer: ");

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
	LightAttenuation lightAttenuation;

	lightAttenuation.factors[0] = 2.5f;
	lightAttenuation.factors[1] = 0.2f;
	lightAttenuation.factors[2] = 1.0f;

#if USE_ALL_SHADER_PROGRAMS
	for(ShaderProgram* program : shaderPrograms)
		program->SetLightAttenuationFactors(lightAttenuation);
#else
	currentShaderProgram->SetLightAttenuationFactors(lightAttenuation);
#endif

	PointLights pointlights;

	lightIntensity = 15.0f;

	pointlights.lightCount = 1;
	for(int i = 0; i < MAX_POINT_LIGHTS; i++)
		pointlights.lights[i].w = lightIntensity;

	lightRotationRadius = 5.0f;
	lightMinHeight = 1.0f;
	lightMaxHeight = 9.5f;

	lightVerticalSpeed = 0.0005f;
	lightHorizontalSpeed = 0.0005f;

	lightSinValMult = 2.0f;
	lightOtherSinValMult = 1.0f;

	numberOfLights = 1;

	auto numberOfLightsCommand = new CommandGetSet<int>("numberOfLights", &numberOfLights);
	auto lightRotationRadiusCommand = new CommandGetSet<float>("lightRotationRadius", &lightRotationRadius);
	auto lightIntensityCommand = new CommandGetSet<float>("lightIntensity", &lightIntensity);
	auto lightMinHeightCommand = new CommandGetSet<float>("lightMinHeight", &lightMinHeight);
	auto lightMaxHeightCommand = new CommandGetSet<float>("lightMaxHeight", &lightMaxHeight);
	auto lightVerticalSpeedCommand = new CommandGetSet<float>("lightVerticalSpeed", &lightVerticalSpeed);
	auto lightHorizontalSpeedCommand = new CommandGetSet<float>("lightHorizontalSpeed", &lightHorizontalSpeed);
	auto lightSinValMultCommand = new CommandGetSet<float>("lightSinValMult", &lightSinValMult);
	auto lightOtherSinValMultCommand = new CommandGetSet<float>("lightOtherSinValMult", &lightOtherSinValMult);

	if(!console.AddCommand(numberOfLightsCommand))
		delete numberOfLightsCommand;
	if(!console.AddCommand(lightRotationRadiusCommand))
		delete lightRotationRadiusCommand;
	if(!console.AddCommand(lightIntensityCommand))
		delete lightIntensityCommand;
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

	return true;
}

bool MulticoreWindow::InitGraphs()
{
	std::vector<TrackDescriptor> perFrameTracks{ TrackDescriptor(10, 1.0f) };
	std::vector<TrackDescriptor> perSecondTracks{ TrackDescriptor(0.1f, 1.0f) };
	std::vector<std::string> perFrameTrackNames{ "Delta", "Primary", "Intersect", "Shade" };
	std::vector<std::string> perSecondTrackNames{ "Primary", "Intersect", "Shade" };

	LogErrorReturnFalse(perFrameGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(0, 720 - 128), DirectX::XMINT2(256, 128), 1000.0f, 1, width, height, true), "Couldn't initialize per frame graph: ");
	LogErrorReturnFalse(perSecondGraph.Init(device.get(), deviceContext.get(), &contentManager, DirectX::XMINT2(perFrameGraph.GetBackgroundWidth() + 256, 720 - 128), DirectX::XMINT2(256, 128), 10000.0f, 1, width, height, true), "Couldn't initialize per second graph: ");

	LogErrorReturnFalse(perFrameGraph.AddTracks(perFrameTrackNames, perFrameTracks), "Couldn't add tracks to per frame graph: ");
	LogErrorReturnFalse(perSecondGraph.AddTracks(perSecondTrackNames, perSecondTracks), "Couldn't add tracks to per second graph: ");

	return true;
}

bool MulticoreWindow::InitRoom()
{
	//////////////////////////////////////////////////
	//Spheres
	//////////////////////////////////////////////////
	std::vector<DirectX::XMFLOAT4> spheres;
	std::vector<DirectX::XMFLOAT4> sphereColors;

	float heightIncrease = 0.2f;

	float rotationValue = 0.0f;
	float heightValue = 0.0f;

	float radius = 50.0f;

	for(int i = 0; i < 0; ++i)
	{
		DirectX::XMFLOAT4 newSphere;
		DirectX::XMFLOAT4 newColor;

		newSphere = DirectX::XMFLOAT4(std::cosf(rotationValue) * radius, 0.0f, std::sinf(rotationValue) * radius, 2.0f);
		if(i % 3 == 0)
			newColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.8f);
		else if(i % 3 == 1)
			newColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.8f);
		else if(i % 3 == 2)
			newColor = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.8f);

		rotationValue += DirectX::XM_2PI / 64.0f;
		heightValue += heightIncrease;

#if USE_ALL_SHADER_PROGRAMS
		for(ShaderProgram* program : shaderPrograms)
			program->AddSphere(newSphere, newColor);
#else
		currentShaderProgram->AddSphere(newSphere, newColor);
#endif
	}



#if USE_ALL_SHADER_PROGRAMS
	for(ShaderProgram* program : shaderPrograms)
	{
		program->AddSphere(DirectX::XMFLOAT4(0.0f, 0.0f, 100.0f, 50.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
		program->AddSphere(DirectX::XMFLOAT4(0.0f, 0.0f, 40.0f, 10.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f));
	}
#else
	//currentShaderProgram->AddSphere(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 10.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
#endif

	//////////////////////////////////////////////////
	//Triangles
	//////////////////////////////////////////////////
	for(int z = 0; z < 2; z++)
	{
		for(int y = 0; y < 2; y++)
		{
			for(int x = 0; x < 2; x++)
#if USE_ALL_SHADER_PROGRAMS
				for(ShaderProgram* program : shaderPrograms)
					program->AddOBJ("sword.obj", DirectX::XMFLOAT3(-1.0f + x * 2.0f, -1.0f + y * 2.0f, -2.5f + z * 5.0f));
			//program->AddOBJ("sword.obj", DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
#else
				currentShaderProgram->AddOBJ("sword.obj", DirectX::XMFLOAT3(-1.0f + x * 2.0f, -1.0f + y * 2.0f, -2.5f + z * 5.0f));
#endif
		}
	}

	//currentShaderProgram->AddOBJ("sword.obj", DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

	return true;
}

bool MulticoreWindow::InitBezier()
{
	std::vector<CameraKeyFrame> cameraKeyFrames = cinematicCamera.GetFrames();

	//Vertices
	LogErrorReturnFalse(bezierVertexBuffer.Create<BezierVertex>(device.get(), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, MAX_BEZIER_LINES * 2), "Couldn't create bezier vertex buffer: ");

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

	domainShaderBinds.AddResource(viewProjMatrixBuffer, 0);

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
	PointLights newData;

	newData.lightCount = numberOfLights;

	float angleIncrease = DirectX::XM_2PI / newData.lightCount * lightOtherSinValMult;
	float heightIncrease = DirectX::XM_2PI / newData.lightCount * lightSinValMult;

	for(int i = 0; i < newData.lightCount; i++)
	{
		newData.lights[i].x = std::cosf(lightOtherSinVal + angleIncrease * i) * lightRotationRadius;
		newData.lights[i].y = ((1.0f + std::sinf(lightSinVal + heightIncrease * i)) * 0.5f) * (lightMaxHeight - lightMinHeight) + lightMinHeight;
		newData.lights[i].z = std::sinf(lightOtherSinVal + angleIncrease * i) * lightRotationRadius;
		newData.lights[i].w = lightIntensity;
	}

	currentShaderProgram->SetPointLights(newData);
}

void MulticoreWindow::DrawUpdateMVP()
{
	DirectX::XMFLOAT4X4 viewMatrix = currentCamera->GetViewMatrix();
	DirectX::XMFLOAT4X4 projectionMatrix = currentCamera->GetProjectionMatrix();
	DirectX::XMFLOAT4X4 viewProjectionMatrix;
	//DirectX::XMFLOAT4X4 viewProjectionMatrixInverse;

	DirectX::XMMATRIX xmViewMatrix = DirectX::XMLoadFloat4x4(&viewMatrix);
	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMLoadFloat4x4(&projectionMatrix);

	DirectX::XMMATRIX xmViewProjMatrix = DirectX::XMMatrixMultiplyTranspose(xmViewMatrix, xmProjectionMatrix);

	DirectX::XMStoreFloat4x4(&viewProjectionMatrix, xmViewProjMatrix);

	currentShaderProgram->SetViewProjMatrix(viewProjectionMatrix);
	currentShaderProgram->SetCameraPosition(currentCamera->GetPosition());

	//DirectX::XMStoreFloat4x4(&viewProjectionMatrixInverse, DirectX::XMMatrixInverse(nullptr, xmViewProjMatrix));

	viewProjMatrixBuffer.Update(deviceContext.get(), &viewProjectionMatrix);

	DirectX::XMFLOAT4X4 projMatrixTranspose = DirectX::XMStoreFloat4x4(DirectX::XMMatrixTranspose(xmProjectionMatrix));
	projMatrixBuffer.Update(deviceContext.get(), &projMatrixTranspose);
	//bezierViewProjMatrixBuffer.Update(deviceContext.get(), &xmViewProjMatrix);
}

void MulticoreWindow::DrawTransferDepthBuffer()
{
	UINT stride = sizeof(DirectX::XMFLOAT2);
	UINT offset = 0;

	ID3D11Buffer* buffer = fullscreenQuadVertexBuffer.GetBuffer();

	deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->OMSetBlendState(BlendStates::singleOff, blendFactor, 0xFFFFFFFF);

	transferDepthPixelShader.Bind(deviceContext.get());
	transferDepthVertexShader.Bind(deviceContext.get());

	deviceContext->Draw(6, 0);

	transferDepthPixelShader.Unbind(deviceContext.get());
	transferDepthVertexShader.Unbind(deviceContext.get());
}

void MulticoreWindow::DrawBulbs()
{
	PointLights pointslights = currentShaderProgram->GetPointLights();

	for(int i = 0, end = std::min(pointslights.lightCount, 10); i < end; i++)
	{
		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixTranslation(pointslights.lights[i].x, pointslights.lights[i].y, pointslights.lights[i].z);
		DirectX::XMStoreFloat4x4(&bulbInstanceData[i].matrix, worldMatrix);
	}

	bulbInstanceBuffer.Update(deviceContext.get(), &bulbInstanceData);

	ID3D11Buffer* bulbBuffers[] = { viewProjMatrixBuffer.GetBuffer() };
	deviceContext->VSSetConstantBuffers(0, 1, bulbBuffers);

	UINT bulbStrides[] = { sizeof(BulbVertex), sizeof(Float4x4BufferData) };
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
	deviceContext->DrawInstanced(6, pointslights.lightCount, 0, 0);
	bulbVertexShader.Unbind(deviceContext.get());
	bulbPixelShader.Unbind(deviceContext.get());

	bulbVertexBuffers[0] = nullptr;
	bulbVertexBuffers[1] = nullptr;

	deviceContext->IASetVertexBuffers(0, 2, bulbVertexBuffers, bulbStrides, bulbOffsets);
}

void MulticoreWindow::DrawBezier()
{
	ID3D11Buffer* bulbBuffers[] = { viewProjMatrixBuffer.GetBuffer() };
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

/*
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

	/ *for(int i = pointLightBufferData.lightCount, end = newCount; i < newCount; ++i)
		pointLightBufferData.lights[i].w = lightIntensity;

	pointLightBufferData.lightCount = newCount;* /

	return "lightCount set";
}*/

/*
Argument MulticoreWindow::ReloadRaytraceShaders(const std::vector<Argument>& argument)
{
	currentshader

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
}*/

bool MulticoreWindow::CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT format /*= DXGI_FORMAT_R32G32B32A32_FLOAT*/)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = format;
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

	bezierVertexCount = static_cast<int>(vertices.size());
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

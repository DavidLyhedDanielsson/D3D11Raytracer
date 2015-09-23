#include "MulticoreWindow.h"

#include <thread>
#include <vector>
#include <functional>

#include <DXLib/Logger.h>
#include <DXLib/input.h>

#include <DXConsole/console.h>

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, samplerState(nullptr, COMUniqueDeleter)
	, backbufferUAV(nullptr, COMUniqueDeleter)
	, vertexBuffer(nullptr, COMUniqueDeleter)
	, indexBuffer(nullptr, COMUniqueDeleter)
	, viewProjMatrixBuffer(nullptr, COMUniqueDeleter)
	, viewProjInverseBuffer(nullptr, COMUniqueDeleter)
	, sphereBuffer(nullptr, COMUniqueDeleter)
	, rayDirectionUAV(nullptr, COMUniqueDeleter)
	, rayPositionUAV(nullptr, COMUniqueDeleter)
	, rayDirectionSRV(nullptr, COMUniqueDeleter)
	, rayPositionSRV(nullptr, COMUniqueDeleter)
	, primaryRayGenerator("main", "cs_5_0")
	, rayTracer("main", "cs_5_0")
	, vertexShader("main", "vs_5_0")
	, pixelShader("main", "ps_5_0")
	, drawConsole(false)
{

}

MulticoreWindow::~MulticoreWindow()
{}

bool MulticoreWindow::Init()
{
	ID3D11Texture2D* backBufferDumb = nullptr;
	HRESULT hRes = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferDumb));
	COMUniquePtr<ID3D11Texture2D> backBuffer(backBufferDumb, COMUniqueDeleter);
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

	std::string errorString = primaryRayGenerator.CreateFromFile("PrimaryRayGenerator.hlsl", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	errorString = rayTracer.CreateFromFile("RayTracer.hlsl", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	std::vector<BUFFER_DATA_TYPES> matrixBuffer = { BUFFER_DATA_TYPES::MAT4X4 };

	viewProjMatrixBuffer.reset(CreateBuffer(matrixBuffer, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE));
	if(viewProjMatrixBuffer == nullptr)
		return false;

	viewProjInverseBuffer.reset(CreateBuffer(matrixBuffer, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE));
	if(viewProjInverseBuffer == nullptr)
		return false;

	SphereBuffer sphereBufferData;
	for(int z = 0; z < 4; ++z)
	{
		for(int y = 0; y < 4; ++y)
		{
			for(int x = 0; x < 4; ++x)
			{
				sphereBufferData.spheres[z * 4 * 4 + y * 4 + x] = DirectX::XMFLOAT4(x * 2.5, y * 2.5f, z * 2.5f, 1.0f);
			}
		}
	}

	sphereBuffer.reset(CreateBuffer(sizeof(float) * 4 * 64, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, static_cast<D3D11_CPU_ACCESS_FLAG>(0), &sphereBufferData));
	if(sphereBuffer == nullptr)
		return false;

	ID3D11UnorderedAccessView* rayPositionUAVDumb = nullptr;
	ID3D11ShaderResourceView* rayPositionSRVDumb = nullptr;

	if(!CreateUAVSRVCombo(width, height, &rayPositionUAVDumb, &rayPositionSRVDumb))
		return false;

	rayPositionUAV.reset(rayPositionUAVDumb);
	rayPositionSRV.reset(rayPositionSRVDumb);

	ID3D11UnorderedAccessView* rayDirectionUAVDumb = nullptr;
	ID3D11ShaderResourceView* rayDirectionSRVDumb = nullptr;

	if(!CreateUAVSRVCombo(width, height, &rayDirectionUAVDumb, &rayDirectionSRVDumb))
		return false;

	rayDirectionUAV.reset(rayDirectionUAVDumb);
	rayDirectionSRV.reset(rayDirectionSRVDumb);

	//Sampler
	D3D11_SAMPLER_DESC samplerDesc;
	memset(&samplerDesc, 0, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	ID3D11SamplerState* samplerStateDumb;
	hRes = device->CreateSamplerState(&samplerDesc, &samplerStateDumb);
	samplerState.reset(samplerStateDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create default sampler state");
		return false;
	}

	Input::Init(hWnd);
	Input::RegisterKeyCallback(std::bind(&MulticoreWindow::KeyEvent, this, std::placeholders::_1));
	Input::RegisterMouseButtonCallback(std::bind(&MulticoreWindow::MouseEvent, this, std::placeholders::_1));
	Input::RegisterCharCallback(std::bind(&MulticoreWindow::CharEvent, this, std::placeholders::_1));
	Input::RegisterScrollCallback(std::bind(&MulticoreWindow::ScrollEvent, this, std::placeholders::_1));

	//Content manager
	contentManager.Init(device.get());
	calibri16 = contentManager.Load<CharacterSet>("calibri16");

	//Sprite renderer
	spriteRenderer.Init(device.get(), deviceContext.get(), &contentManager, 1280, 720);

	//Console
	auto style = std::shared_ptr<GUIStyle>(console.GenerateDoomStyle(&contentManager));
	auto background = std::unique_ptr<GUIBackground>(console.GenerateDoomStyleBackground(&contentManager));
	auto backgroundStyle = std::shared_ptr<GUIStyle>(console.GenerateDoomStyleBackgroundStyle(&contentManager));

	console.Init(Rect(0.0f, 0.0f, 1280.0f, 300.0f), style, background, backgroundStyle);
	console.Autoexec();

	guiManager.AddContainer(&console);

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

	Timer gameTimer;
	gameTimer.Start();

	std::chrono::nanoseconds accumulatedDelta(0);
	int iterations = 0;

	std::chrono::nanoseconds targetFrametime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1) / 65.0f);

	Timer frameLimiter;
	frameLimiter.Start();

	bool run = true;

	while(run)
	{
		run = PeekMessages();
		Input::Update();

		if(!paused)
		{
			frameLimiter.Reset();

			gameTimer.UpdateDelta();
			accumulatedDelta += gameTimer.GetDelta();
			++iterations;

			if(accumulatedDelta.count() >= 1000000000)
			{
				std::chrono::nanoseconds averageDuration(static_cast<unsigned long long>(accumulatedDelta.count() / static_cast<float>(iterations)));

				SetWindowText(hWnd, (std::to_string(averageDuration.count() * 1e-9f) + ", " + std::to_string(iterations)).c_str());

				accumulatedDelta = accumulatedDelta - std::chrono::nanoseconds(1000000000);
				iterations = 0;
			}

			Update(gameTimer.GetDelta());
			Draw();

			frameLimiter.Stop();
			if(frameLimiter.GetTime() < targetFrametime)
				std::this_thread::sleep_for(targetFrametime - frameLimiter.GetTime());
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void MulticoreWindow::Update(std::chrono::nanoseconds delta)
{
	double fracDelta = delta.count() * 1e-6;

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

		float cameraSpeed = 0.005f * fracDelta;

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
	float colors[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRenderTarget.get(), colors);

	//Update viewProjMatrix and viewProjInverseMatrix
	DirectX::XMFLOAT4X4 viewMatrix = camera.GetViewMatrix();
	DirectX::XMFLOAT4X4 projectionMatrix = camera.GetProjectionMatrix();

	DirectX::XMMATRIX xmViewMatrix = DirectX::XMLoadFloat4x4(&viewMatrix);
	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMLoadFloat4x4(&projectionMatrix);

	DirectX::XMMATRIX xmViewProjMatrix = DirectX::XMMatrixMultiplyTranspose(xmViewMatrix, xmProjectionMatrix);

	ViewProjBuffer viewProjBuffer;

	DirectX::XMStoreFloat4x4(&viewProjBuffer.viewProjMatrixInverse, DirectX::XMMatrixInverse(nullptr, xmViewProjMatrix));

	D3D11_MAPPED_SUBRESOURCE mappedViewProjMatrixBuffer;
	if(FAILED(deviceContext->Map(viewProjInverseBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedViewProjMatrixBuffer)))
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't map buffer");
		return;
	}

	std::memcpy(mappedViewProjMatrixBuffer.pData, &viewProjBuffer, sizeof(float) * 32);

	deviceContext->Unmap(viewProjInverseBuffer.get(), 0);

	//Ray shaders
	//Unbind backbuffer and bind it as resource
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	//Primary rays
	ID3D11UnorderedAccessView* uav[] = { rayPositionUAV.get(), rayDirectionUAV.get() };
	deviceContext->CSSetUnorderedAccessViews(0, 2, uav, nullptr);

	ID3D11Buffer* viewProjInverseBufferDumb = viewProjInverseBuffer.get();
	deviceContext->CSSetConstantBuffers(0, 1, &viewProjInverseBufferDumb);

	primaryRayGenerator.Bind(deviceContext.get());
	deviceContext->Dispatch(40, 45, 1);
	primaryRayGenerator.Unbind(deviceContext.get());

	viewProjInverseBufferDumb = nullptr;
	deviceContext->CSSetConstantBuffers(0, 1, &viewProjInverseBufferDumb);

	uav[0] = nullptr;
	uav[1] = nullptr;
	deviceContext->CSSetUnorderedAccessViews(0, 2, uav, nullptr);

	//Ray trace
	ID3D11SamplerState* samplerStateDumb = samplerState.get();
	deviceContext->CSSetSamplers(0, 1, &samplerStateDumb);

	ID3D11ShaderResourceView* resourceViews[] { rayPositionSRV.get(), rayDirectionSRV.get() };
	deviceContext->CSSetShaderResources(0, 2, resourceViews);

	uav[0] = backbufferUAV.get();
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

	ID3D11Buffer* sphereBufferDumb = sphereBuffer.get();
	deviceContext->CSSetConstantBuffers(0, 1, &sphereBufferDumb);

	rayTracer.Bind(deviceContext.get());
	deviceContext->Dispatch(40, 45, 1);
	rayTracer.Unbind(deviceContext.get());

	sphereBufferDumb = nullptr;
	deviceContext->CSSetConstantBuffers(0, 1, &sphereBufferDumb);

	//Unbind backbuffer resource and bind it as render target
	uav[0] = nullptr;
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

	resourceViews[0] = nullptr;
	resourceViews[1] = nullptr;
	deviceContext->CSSetShaderResources(0, 2, resourceViews);

	renderTargets[0] = backBufferRenderTarget.get();
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	//Normal forward drawing
	deviceContext->ClearDepthStencilView(depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	deviceContext->RSSetState(rasterizerState.get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	spriteRenderer.Begin();

	guiManager.Draw(&spriteRenderer);

	spriteRenderer.End();

	swapChain->Present(0, 0);
}

LRESULT MulticoreWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_ACTIVATE:
			if(LOWORD(wParam) == WA_INACTIVE)
				paused = true;
			else
				paused = false;
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

		if(keyCode.key == VK_F1)
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

ID3D11Buffer* MulticoreWindow::CreateBuffer(const std::vector<BUFFER_DATA_TYPES>& bufferDataTypes, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
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

	ID3D11Buffer* returnBuffer = nullptr;
	if(FAILED(device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &returnBuffer)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create buffer with size " + std::to_string(size));
		return nullptr;
	}

	return returnBuffer;
}

ID3D11Buffer* MulticoreWindow::CreateBuffer(UINT size, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
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

	ID3D11Buffer* returnBuffer = nullptr;
	if(FAILED(device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &returnBuffer)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create buffer with size " + std::to_string(size));
		return nullptr;
	}

	return returnBuffer;
}

ID3D11Buffer* MulticoreWindow::CreateBuffer(BUFFER_DATA_TYPES bufferDataType, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/)
{
	std::vector<BUFFER_DATA_TYPES> bufferDataTypes = { bufferDataType };

	return CreateBuffer(bufferDataTypes, usage, bindFlags, cpuAccess);
}

bool MulticoreWindow::CreateUAVSRVCombo(int width, int height, ID3D11UnorderedAccessView** uav, ID3D11ShaderResourceView** srv)
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

	COMUniquePtr<ID3D11Texture2D> texture(nullptr, COMUniqueDeleter);

	ID3D11Texture2D* textureDumb = nullptr;
	texture.reset(textureDumb);
	HRESULT hRes = device->CreateTexture2D(&desc, nullptr, &textureDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
		return false;
	}

	if(FAILED(device->CreateUnorderedAccessView(textureDumb, nullptr, uav)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create combo UAV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
		return false;
	}

	if(FAILED(device->CreateShaderResourceView(textureDumb, nullptr, srv)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create combo SRV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height));
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
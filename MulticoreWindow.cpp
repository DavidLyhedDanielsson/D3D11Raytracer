#include "MulticoreWindow.h"

#include <thread>
#include <vector>

#include "Logger.h"

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, backbufferUAV(nullptr, COMUniqueDeleter)
	, vertexBuffer(nullptr, COMUniqueDeleter)
	, computeShader("main", "cs_5_0")
	, vertexShader("main", "vs_5_0")
	, pixelShader("main", "ps_5_0")
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

	ID3D11UnorderedAccessView* backbufferUAVDumb;
	hRes = device->CreateUnorderedAccessView(backBuffer.get(), nullptr, &backbufferUAVDumb);
	backbufferUAV.reset(backbufferUAVDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create UAV from backBuffer");
		return false;
	}

	std::string errorString = computeShader.CreateFromFile("ComputeShader.cso", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	errorString = vertexShader.CreateFromFile("VertexShader.cso", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	errorString = pixelShader.CreateFromFile("PixelShader.cso", device.get());
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return false;
	}

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(Vertex) * 3;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	std::vector<Vertex> data;
	data.emplace_back(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	data.emplace_back(DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));
	data.emplace_back(DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));

	D3D11_SUBRESOURCE_DATA dataDesc;
	ZeroMemory(&dataDesc, sizeof(dataDesc));

	dataDesc.pSysMem = &data[0];

	ID3D11Buffer* vertexBufferDumb = nullptr;
	hRes = device->CreateBuffer(&desc, &dataDesc, &vertexBufferDumb);
	vertexBuffer.reset(vertexBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create vertex buffer");
		return false;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = false;
	rasterizerDesc.DepthBiasClamp = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0;
	rasterizerDesc.DepthClipEnable = false;
	rasterizerDesc.ScissorEnable = true;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = false;

	device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);

	return true;
}

void MulticoreWindow::Run()
{
	if(!Init())
		return;

	Timer gameTimer;
	gameTimer.Start();

	std::chrono::nanoseconds accumulatedDelta;
	int iterations = 0;

	while(!PeekMessages())
	{
		if(!paused)
		{
			gameTimer.UpdateDelta();

			accumulatedDelta += gameTimer.GetDelta();
			++iterations;

			if(accumulatedDelta.count() >= 1000000000)
			{
				std::chrono::nanoseconds averageDuration(static_cast<unsigned long long>(accumulatedDelta.count() / static_cast<float>(iterations)));

				SetWindowText(hWnd, std::to_string(averageDuration.count() * 1e-9f).c_str());

				accumulatedDelta *= 0;
				iterations = 0;
			}

			Update(gameTimer.GetDelta());
			Draw();
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void MulticoreWindow::Update(std::chrono::nanoseconds delta)
{

}

void MulticoreWindow::Draw()
{
	//Unbind backbuffer and bind it as resource
	/*ID3D11RenderTargetView* renderTargets[] = { nullptr };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	ID3D11UnorderedAccessView* uav[] = { backbufferUAV.get() };
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

	computeShader.Bind(deviceContext.get());

	deviceContext->Dispatch(40, 45, 1);

	computeShader.Unbind(deviceContext.get());

	//Unbind backbuffer resource and bind it as render target
	uav[0] = nullptr;
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);*/

	deviceContext->ClearDepthStencilView(depthStencilView.get(), 0, 0.0f, 0);

	float colors[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	deviceContext->ClearRenderTargetView(backBufferRenderTarget.get(), colors);

	ID3D11RenderTargetView* renderTargets[] = { backBufferRenderTarget.get() };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	ID3D11Buffer* buffer = vertexBuffer.get();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	D3D11_VIEWPORT viewPort;
	viewPort.Width = 1280.0f;
	viewPort.Height = 720.0f;
	viewPort.TopLeftX = 0.0f;
	viewPort.TopLeftY = 0.0f;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;

	deviceContext->RSSetState(rasterizerState);
	deviceContext->RSSetViewports(1, &viewPort);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

	vertexShader.Bind(deviceContext.get());
	pixelShader.Bind(deviceContext.get());

	deviceContext->Draw(3, 0);

	vertexShader.Unbind(deviceContext.get());
	pixelShader.Unbind(deviceContext.get());

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

void MulticoreWindow::KeyDown(WPARAM keyCode)
{
	if(keyCode == VK_ESCAPE)
		PostQuitMessage(0);
}

void MulticoreWindow::KeyRepeat(WPARAM keyCode)
{
	
}

void MulticoreWindow::KeyUp(WPARAM keyCode)
{
	
}

void MulticoreWindow::MouseDown(WPARAM keyCode)
{
	
}

void MulticoreWindow::MouseUp(WPARAM keyCode)
{

}
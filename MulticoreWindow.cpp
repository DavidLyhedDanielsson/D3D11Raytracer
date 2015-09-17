#include "MulticoreWindow.h"

#include "Timer.h"

#include <thread>

#include "Logger.h"

MulticoreWindow::MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: DX11Window(hInstance, nCmdShow, width, height)
	, paused(false)
	, backbufferUAV(nullptr, COMUniqueDeleter)
{

}

MulticoreWindow::~MulticoreWindow()
{}

bool MulticoreWindow::Init()
{
	ID3D11Texture2D* backBufferDumb;
	if(FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferDumb))))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't get backBuffer from swapChain");
		return false;
	}

	COMUniquePtr<ID3D11Texture2D> backBuffer(backBufferDumb, COMUniqueDeleter);

	ID3D11UnorderedAccessView* backbufferUAVDumb;
	if(FAILED(device->CreateUnorderedAccessView(backBuffer.get(), nullptr, &backbufferUAVDumb)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create UAV from backBuffer");
		return false;
	}

	backbufferUAV.reset(backbufferUAVDumb);

	return true;
}

void MulticoreWindow::Run()
{
	if(!Init())
		return;

	Timer gameTimer;
	gameTimer.Start();

	while(!PeekMessages())
	{
		if(!paused)
		{
			gameTimer.UpdateDelta();
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
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

	ID3D11UnorderedAccessView* uav[] = { backbufferUAV.get() };
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);
	
	uav[0] = nullptr;
	deviceContext->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

	renderTargets[0] = backBufferRenderTarget.get();
	deviceContext->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

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

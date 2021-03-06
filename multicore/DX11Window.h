#pragma once

#include <DXLib/Common.h>

#include <string>
#include <memory>

class DX11Window
{
public:
	DX11Window(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height);
	virtual ~DX11Window();

	virtual std::string CreateDXWindow(HINSTANCE hInstance, int nCmdShow, int targetMonitor = 0);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool PeekMessages();

protected:
	HINSTANCE hInstance;
	HWND hWnd;

	COMUniquePtr<ID3D11Device> device;
	COMUniquePtr<ID3D11DeviceContext> deviceContext;
	COMUniquePtr<IDXGISwapChain> swapChain;
	COMUniquePtr<ID3D11RenderTargetView> backBufferRenderTarget;
	COMUniquePtr<ID3D11DepthStencilView> depthStencilView;
	COMUniquePtr<ID3D11RasterizerState> rasterizerState;

	UINT width;
	UINT height;

	virtual void KeyDown(WPARAM keyCode);
	virtual void KeyRepeat(WPARAM keyCode);
	virtual void KeyUp(WPARAM keyCode);
	virtual void MouseDown(WPARAM keyCode);
	virtual void MouseUp(WPARAM keyCode);

private:
	std::string CreateWindowsWindow(int nCmdShow, int targetMonitor);
	std::string CreateSwapChain();
	std::string CreateRenderTargetView();
	std::string CreateDepthStencilBuffer();
	
	void SetScissorRect();
	void SetViewport();
	void SetRasterizerState();

	std::string CreateDevice();
};

static DX11Window* window = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
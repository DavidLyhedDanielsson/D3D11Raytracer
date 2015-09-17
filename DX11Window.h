#pragma once

#include "stdafx.h"

#include <string>
#include <memory>

class DX11Window
{
public:
	DX11Window(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height);
	virtual ~DX11Window();

	virtual std::string CreateDXWindow(HINSTANCE hInstance, int nCmdShow);
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

	/*std::unique_ptr<ID3D11Device, decltypeCOMDeleter> device;
	std::unique_ptr<ID3D11DeviceContext, decltypeCOMDeleter> deviceContext;
	std::unique_ptr<IDXGISwapChain, decltypeCOMDeleter> swapChain;
	std::unique_ptr<ID3D11RenderTargetView, decltypeCOMDeleter> backBufferRenderTarget;
	std::unique_ptr<ID3D11DepthStencilView, decltypeCOMDeleter> depthStencilView;*/

	UINT width;
	UINT height;

	virtual void KeyDown(WPARAM keyCode);
	virtual void KeyRepeat(WPARAM keyCode);
	virtual void KeyUp(WPARAM keyCode);
	virtual void MouseDown(WPARAM keyCode);
	virtual void MouseUp(WPARAM keyCode);

private:
	std::string CreateWindowsWindow(int nCmdShow);
	std::string CreateSwapChain();
	std::string CreateRenderTargetView();
	std::string CreateDepthStencilBuffer();

	std::string CreateDevice();
};

static DX11Window* window = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
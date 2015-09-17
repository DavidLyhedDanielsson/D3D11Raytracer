#pragma once

#include "DX11Window.h"

#include <chrono>

#include "ComputeShader.h"

class MulticoreWindow :
	public DX11Window
{
public:
	MulticoreWindow(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height);
	~MulticoreWindow();

	void Run();

	bool Init();
	void Update(std::chrono::nanoseconds delta);
	void Draw();

	LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	virtual void KeyDown(WPARAM keyCode) override;
	virtual void KeyRepeat(WPARAM keyCode) override;
	virtual void KeyUp(WPARAM keyCode) override;
	virtual void MouseDown(WPARAM keyCode) override;
	virtual void MouseUp(WPARAM keyCode) override;

private:
	bool paused;

	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;

	ComputeShader computeShader;
};


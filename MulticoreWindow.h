#pragma once

#include "DX11Window.h"

#include <chrono>

#include "ComputeShader.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Timer.h"

#include <DirectXMath.h>

namespace
{
	struct Vertex
	{
		Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color)
			: position(position)
			, color(color)
		{

		}

		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 color;
		DirectX::XMFLOAT2 padding;
	};
}

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
	COMUniquePtr<ID3D11Buffer> vertexBuffer;

	ID3D11RasterizerState* rasterizerState;

	ComputeShader computeShader;
	VertexShader vertexShader;
	PixelShader pixelShader;
};


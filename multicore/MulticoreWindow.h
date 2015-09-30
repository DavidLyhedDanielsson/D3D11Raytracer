#pragma once

#include "DX11Window.h"

#include <chrono>
#include <unordered_set>

#include <DXLib/VertexShader.h>
#include <DXLib/PixelShader.h>
#include <DXLib/Timer.h>
#include <DXLib/spriteRenderer.h>
#include <DXLib/keyState.h>
#include <DXLib/D3D11Timer.h>

#include <DXConsole/guiManager.h>
#include <DXConsole/Console.h>
#include <DXConsole/Console.h>

#include <DirectXMath.h>

#include "FPSCamera.h"
#include "Graph.h"
#include "ComputeShader.h"

enum class BUFFER_DATA_TYPES
{
	MAT4X4
	, INT
	, FLOAT1
	, FLOAT2
	, FLOAT3
	, FLOAT4
};

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

	struct ViewProjBuffer 
	{
		DirectX::XMFLOAT4X4 viewProjMatrixInverse;
	};

	struct PointlightBuffer
	{
		DirectX::XMFLOAT4 lights[10];
		int lightCount;
	};

	struct SphereBuffer
	{
		DirectX::XMFLOAT4 spheres[64];
		int sphereCount;
	};

	struct TriangleBuffer
	{
		DirectX::XMFLOAT4 triangles[64 * 3];
		int triangleCount;
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

	void KeyEvent(const KeyState& keyCode);
	void MouseEvent(const KeyState& keyCode);
	void CharEvent(int character);
	void ScrollEvent(int distance);

private:
	bool paused;

	FPSCamera camera;

	std::unordered_set<int> keyMap;

	COMUniquePtr<ID3D11SamplerState> samplerState;

	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;
	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;

	COMUniquePtr<ID3D11Buffer> viewProjMatrixBuffer;
	COMUniquePtr<ID3D11Buffer> viewProjInverseBuffer;
	COMUniquePtr<ID3D11Buffer> lightBuffer;
	COMUniquePtr<ID3D11Buffer> sphereBuffer;
	COMUniquePtr<ID3D11Buffer> triangleBuffer;

	COMUniquePtr<ID3D11UnorderedAccessView> rayDirectionUAV[2];
	COMUniquePtr<ID3D11UnorderedAccessView> rayPositionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayDirectionSRV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayPositionSRV[2];

	COMUniquePtr<ID3D11UnorderedAccessView> rayNormalUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayNormalSRV;

	std::vector<unsigned int> indexData;

	ComputeShader primaryRayGenerator;
	ComputeShader trace;
	ComputeShader shade;

	VertexShader vertexShader;
	PixelShader pixelShader;

	ContentManager contentManager;
	SpriteRenderer spriteRenderer;

	CharacterSet* calibri16;

	GUIManager guiManager;
	
	Timer gameTimer;
	D3D11Timer d3d11Timer;

	Graph cpuGraph;
	Graph gpuGraph;

	Console console;
	bool drawConsole;

	ID3D11Buffer* CreateBuffer(const std::vector<BUFFER_DATA_TYPES>& bufferDataTypes
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);
	ID3D11Buffer* CreateBuffer(BUFFER_DATA_TYPES bufferDataType
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);
	ID3D11Buffer* CreateBuffer(UINT size
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);

	bool CreateUAVSRVCombo(int width, int height, ID3D11UnorderedAccessView** uav, ID3D11ShaderResourceView** srv);

	bool GenerateCubePrimitive(std::vector<unsigned int> &indexData, ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer);
};


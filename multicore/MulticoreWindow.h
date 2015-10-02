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

const static int MAX_POINT_LIGHTS = 10;
const static int MAX_SPHERES = 64;
const static int MAX_TRIANGLES = 64;

namespace
{
	struct Vertex
	{
		Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color)
			: position(position)
			, color(color)
		{

		}

		Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, DirectX::XMFLOAT2 texCoord)
			: position(position)
			, color(color)
			, texCoord(texCoord)
		{

		}

		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 color;
		DirectX::XMFLOAT2 texCoord;
	};

	struct BulbInstanceData
	{
		DirectX::XMFLOAT4X4 worldMatrix;
	};

	struct ViewProjBuffer 
	{
		DirectX::XMFLOAT4X4 viewProjMatrixInverse;
	};

	struct PointlightBuffer
	{
		DirectX::XMFLOAT4 lights[MAX_POINT_LIGHTS];
		int lightCount;
	};

	struct SphereBuffer
	{
		DirectX::XMFLOAT4 spheres[MAX_SPHERES];
		int sphereCount;
	};

	struct TriangleBuffer
	{
		DirectX::XMFLOAT4 triangles[MAX_TRIANGLES * 3];
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

	ID3D11BlendState* billboardBlendState;
	ID3D11SamplerState* billboardSamplerState;
	ID3D11RasterizerState* billboardRasterizerState;

	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;
	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;

	COMUniquePtr<ID3D11Buffer> viewProjMatrixBuffer;
	COMUniquePtr<ID3D11Buffer> viewProjInverseBuffer;
	COMUniquePtr<ID3D11Buffer> sphereBuffer;
	COMUniquePtr<ID3D11Buffer> triangleBuffer;

	COMUniquePtr<ID3D11UnorderedAccessView> rayDirectionUAV[2];
	COMUniquePtr<ID3D11UnorderedAccessView> rayPositionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayDirectionSRV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayPositionSRV[2];

	COMUniquePtr<ID3D11UnorderedAccessView> rayNormalUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayNormalSRV;

	//Point lights
	COMUniquePtr<ID3D11Buffer> pointLightBuffer;
	PointlightBuffer pointLightBufferData;


	std::vector<unsigned int> indexData;

	ComputeShader primaryRayGenerator;
	ComputeShader trace;
	ComputeShader shade;

	VertexShader billboardVertexShader;
	PixelShader billboardPixelShader;
	COMUniquePtr<ID3D11Buffer> bulbProjMatrixBuffer;
	COMUniquePtr<ID3D11Buffer> bulbInstanceBuffer;
	COMUniquePtr<ID3D11Buffer> bulbVertexBuffer;

	BulbInstanceData bulbInstanceData[MAX_POINT_LIGHTS];
	Texture2D* bulbTexture;

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


	bool InitSRVs();
	bool InitRaytraceShaders();

	bool InitBulb();
	bool InitPointLights();
	bool InitGraphs();
	bool InitRoom();

	void InitInput();
	void InitConsole();

	COMUniquePtr<ID3D11Buffer> CreateBuffer(const std::vector<BUFFER_DATA_TYPES>& bufferDataTypes
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);
	COMUniquePtr<ID3D11Buffer> CreateBuffer(BUFFER_DATA_TYPES bufferDataType
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);
	COMUniquePtr<ID3D11Buffer> CreateBuffer(UINT size
		, D3D11_USAGE usage
		, D3D11_BIND_FLAG bindFlags
		, D3D11_CPU_ACCESS_FLAG cpuAccess
		, void* initialData = nullptr);

	bool CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv);

	bool GenerateCubePrimitive(std::vector<unsigned int> &indexData, ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer);
};


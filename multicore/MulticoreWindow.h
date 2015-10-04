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
#include <DXLib/DXBuffer.h>

#include <DXConsole/guiManager.h>
#include <DXConsole/Console.h>
#include <DXConsole/Console.h>

#include <DirectXMath.h>

#include "FPSCamera.h"
#include "Graph.h"
#include "ComputeShader.h"

/*
enum class BUFFER_DATA_TYPES
{
	MAT4X4
	, INT
	, FLOAT1
	, FLOAT2
	, FLOAT3
	, FLOAT4
};*/

const static int MAX_POINT_LIGHTS = 10;
const static int MAX_SPHERES = 64;
const static int MAX_TRIANGLES = 64;

#define LogErrorReturnFalse(functionCall, messagePrefix)				\
{																		\
	std::string errorString = functionCall;								\
	if(!errorString.empty())											\
	{																	\
		Logger::LogLine(LOG_TYPE::FATAL, messagePrefix + errorString);	\
		return false;													\
	}																	\
} 

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

	struct CameraPositionBufferData
	{
		DirectX::XMFLOAT3 position;
	};

	struct Float4x4BufferData 
	{
		DirectX::XMFLOAT4X4 matrix;
	};

	struct LightAttenuationBufferData
	{
		float factors[3];
	};

	struct PointlightBufferData
	{
		DirectX::XMFLOAT4 lights[MAX_POINT_LIGHTS];
		int lightCount;
	};

	struct SphereBufferData
	{
		DirectX::XMFLOAT4 spheres[MAX_SPHERES];
		DirectX::XMFLOAT4 colors[MAX_SPHERES];

		int sphereCount;
	};

	struct TriangleBufferData
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
	int dispatchX;
	int dispatchY;

	bool paused;

	FPSCamera camera;

	std::unordered_set<int> keyMap;

	ID3D11BlendState* billboardBlendState;
	ID3D11SamplerState* billboardSamplerState;
	ID3D11RasterizerState* billboardRasterizerState;

	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;
	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;

	DXBuffer viewProjMatrixBuffer;
	DXBuffer viewProjInverseBuffer;
	DXBuffer sphereBuffer;
	DXBuffer triangleBuffer;

	COMUniquePtr<ID3D11UnorderedAccessView> outputColorUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> outputColorSRV[2];

	COMUniquePtr<ID3D11UnorderedAccessView> rayColorUAV;
	COMUniquePtr<ID3D11UnorderedAccessView> rayDirectionUAV[2];
	COMUniquePtr<ID3D11UnorderedAccessView> rayPositionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayColorSRV;
	COMUniquePtr<ID3D11ShaderResourceView> rayDirectionSRV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayPositionSRV[2];

	COMUniquePtr<ID3D11UnorderedAccessView> rayNormalUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayNormalSRV;

	//Point lights
	DXBuffer pointlightAttenuationBuffer;
	DXBuffer pointLightBuffer;
	DXBuffer cameraPositionBuffer;
	PointlightBufferData pointLightBufferData;
	LightAttenuationBufferData pointlightAttenuationBufferData;

	float lightRadius;

	float lightSinValMult;
	float lightOtherSinValMult;

	float lightRotationRadius;
	float lightMinHeight;
	float lightMaxHeight;

	float lightVerticalSpeed;
	float lightHorizontalSpeed;

	const static int MAX_BOUNCES = 10;

	std::vector<unsigned int> indexData;

	ComputeShader primaryRayGenerator;
	ComputeShader traceShader;
	ComputeShader intersectionShader;
	ComputeShader compositShader;

	VertexShader bulbVertexShader;
	PixelShader bulbPixelShader;
	DXBuffer bulbProjMatrixBuffer;
	DXBuffer bulbInstanceBuffer;
	DXBuffer bulbVertexBuffer;

	Float4x4BufferData bulbInstanceData[MAX_POINT_LIGHTS];
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

	void DrawUpdatePointlights();

	void DrawUpdateMVP();
	void DrawRayPrimary();
	void DrawRayIntersection(int config);
	void DrawRayShading(int config);
	void DrawComposit(int config);

	void DrawBulbs();

	Argument SetNumberOfLights(const std::vector<Argument>& argument);
	Argument SetLightAttenuationFactors(const std::vector<Argument>& argument);
	Argument ReloadRaytraceShaders(const std::vector<Argument>& argument);

	bool CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv);
};


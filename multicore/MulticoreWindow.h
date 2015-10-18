#pragma once

#include "DX11Window.h"

#include <chrono>
#include <unordered_set>

#include <DXLib/VertexShader.h>
#include <DXLib/PixelShader.h>
#include <DXLib/HullShader.h>
#include <DXLib/DomainShader.h>
#include <DXLib/Timer.h>
#include <DXLib/spriteRenderer.h>
#include <DXLib/keyState.h>
#include <DXLib/D3D11Timer.h>
#include <DXLib/DXConstantBuffer.h>
#include <DXLib/FPSCamera.h>
#include <DXLib/CinematicCamera.h>
#include <DXLib/OBJFile.h>
#include <DXLib/DXMath.h>

#include "SharedShaderBuffers.h"

#include <DXConsole/guiManager.h>
#include <DXConsole/Console.h>


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

	struct BezierVertex
	{
		DirectX::XMFLOAT3 beginAnchor;
		DirectX::XMFLOAT3 endAnchor;
		DirectX::XMFLOAT3 beginHandle;
		DirectX::XMFLOAT3 endHandle;
		float lambda; //52 bytes

		uint8_t padding[12]; //12 bytes of padding = 64 bytes
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

	std::unordered_set<int> keyMap;

	//////////////////////////////////////////////////
	//Camera
	//////////////////////////////////////////////////
	bool cinematicCameraMode;

	FPSCamera fpsCamera;
	CinematicCamera cinematicCamera;
	Camera* currentCamera;

	DirectX::XMFLOAT3 cameraLookAt;
	std::vector<DirectX::XMFLOAT3> cameraAnchors;

	//////////////////////////////////////////////////
	//Ray tracing
	//////////////////////////////////////////////////
	const static int MAX_BOUNCES = 20;
	int rayBounces;

	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;

	COMUniquePtr<ID3D11UnorderedAccessView> outputColorUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> outputColorSRV[2];

	//Contains the ray's current color (accumulates over bounces)
	COMUniquePtr<ID3D11UnorderedAccessView> rayColorUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayColorSRV;
	//Contains the ray's current direction (changes with every bounce)
	COMUniquePtr<ID3D11UnorderedAccessView> rayDirectionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayDirectionSRV[2];
	//Contains the ray's current position (changes with every bounce unless normal == 0)
	COMUniquePtr<ID3D11UnorderedAccessView> rayPositionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayPositionSRV[2];

	//Contains the normal at the ray's intersection location
	//Also used to check if the ray didn't hit anything, in that case it will be (0, 0, 0)
	COMUniquePtr<ID3D11UnorderedAccessView> rayNormalUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayNormalSRV;

	//Used to create primary rays
	DXConstantBuffer viewProjInverseBuffer;

	//Spheres to ray trace against
	DXConstantBuffer sphereBuffer;
	DXConstantBuffer triangleVertexBuffer;
	DXConstantBuffer triangleBuffer;

	////////////////////
	//Point lights
	////////////////////
	//Intensity of the light. This is the multiplied with the diffuse factor before dividing with the attenuation factor
	float lightIntensity;

	//Used to calculate light offsets (relative to each other)
	float lightSinValMult;
	float lightOtherSinValMult;

	//Radius of the rotating point lights
	float lightRotationRadius;
	//Top and bottom height of all moving point lights
	float lightMinHeight;
	float lightMaxHeight;

	float lightVerticalSpeed;
	float lightHorizontalSpeed;

	DXConstantBuffer pointlightAttenuationBuffer;
	DXConstantBuffer pointLightBuffer;
	DXConstantBuffer cameraPositionBuffer;
	PointlightBufferData pointLightBufferData;
	LightAttenuationBufferData pointlightAttenuationBufferData;

	////////////////////
	//Shaders
	////////////////////
	ComputeShader primaryRayGenerator;
	ComputeShader traceShader;
	ComputeShader intersectionShader;
	ComputeShader compositShader;

	////////////////////
	//Etc
	////////////////////
	OBJFile* swordOBJ;

	//////////////////////////////////////////////////
	//Forward rendering
	//////////////////////////////////////////////////
	ID3D11BlendState* billboardBlendState;
	ID3D11SamplerState* billboardSamplerState;
	ID3D11RasterizerState* billboardRasterizerState;

	////////////////////
	//Point lights
	////////////////////
	VertexShader bulbVertexShader;
	PixelShader bulbPixelShader;
	DXConstantBuffer bulbViewProjMatrixBuffer;
	DXConstantBuffer bulbInstanceBuffer;
	DXConstantBuffer bulbVertexBuffer;

	Float4x4BufferData bulbInstanceData[MAX_POINT_LIGHTS];
	Texture2D* bulbTexture;

	//////////////////////////////////////////////////
	//Bezier drawing
	//////////////////////////////////////////////////
	const static int MAX_BEZIER_LINES = 50;
	int bezierVertexCount;

	PixelShader bezierPixelShader;
	VertexShader bezierVertexShader;
	HullShader bezierHullShader;
	DomainShader bezierDomainShader;

	DXConstantBuffer bezierViewProjMatrixBuffer;
	DXConstantBuffer bezierVertexBuffer;

	DXConstantBuffer bezierTessFactorBuffer;

	std::vector<BezierVertex> CalcBezierVertices(const std::vector<CameraKeyFrame>& frames) const;
	void UploadBezierFrames();
	Argument SetBezierTessFactors(const std::vector<Argument>& argument);

	//////////////////////////////////////////////////
	//Etc
	//////////////////////////////////////////////////
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


	Argument ResetCamera(const std::vector<Argument>& argument);
	Argument PauseCamera(const std::vector<Argument>& argument);
	Argument StartCamera(const std::vector<Argument>& argument);

	Argument AddCameraFrame(const std::vector<Argument>& argument);
	Argument SetCameraFrame(const std::vector<Argument>& argument);
	Argument RemoveCameraFrame(const std::vector<Argument>& argument);
	Argument PrintCameraFrames(const std::vector<Argument>& argument);
	Argument SetCameraTargetSpeed(const std::vector<Argument>& argument);

	bool InitSRVs();
	bool InitRaytraceShaders();

	bool InitBulb();
	bool InitPointLights();
	bool InitGraphs();
	bool InitRoom();
	void LoadOBJ(VertexBuffer& vertexBuffer, TriangleBuffer& triangleBuffer);
	bool InitBezier();

	void InitInput();
	void InitConsole();

	void DrawUpdatePointlights();

	void DrawUpdateMVP();
	void DrawRayPrimary();
	void DrawRayIntersection(int config);
	void DrawRayShading(int config);
	void DrawComposit(int config);

	void DrawBulbs();
	void DrawBezier();

	Argument SetNumberOfLights(const std::vector<Argument>& argument);
	Argument SetLightAttenuationFactors(const std::vector<Argument>& argument);
	Argument ReloadRaytraceShaders(const std::vector<Argument>& argument);

	bool CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv);

	void AddFace(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max, DirectX::XMFLOAT4 color, VertexBuffer& vertexBuffer, TriangleBuffer& triangleBuffer, bool flipWindingOrder) const;
};


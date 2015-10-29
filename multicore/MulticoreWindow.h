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

#include "SharedShaderConstants.h"

#include <DXConsole/guiManager.h>
#include <DXConsole/Console.h>

#include "Graph.h"
#include "ComputeShader.h"

#define USE_ALL_SHADER_PROGRAMS true

//#define USE_CONSTANT_BUFFER_SHADER_PROGRAM true
//#define USE_STRUCTURED_BUFFER_SHADER_PROGRAM true
//#define USE_AABBSTRUCTUREDBUFFER_SHADER_PROGRAM true


class ShaderProgram;
class ConstantBufferShaderProgram;
class StructuredBufferShaderProgram;
class AABBStructuredBufferShaderProgram;

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
	struct BulbVertex
	{
		BulbVertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color)
			: position(position)
			, color(color)
		{

		}

		BulbVertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, DirectX::XMFLOAT2 texCoord)
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

	struct Float4x4BufferData 
	{
		DirectX::XMFLOAT4X4 matrix;
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
	COMUniquePtr<ID3D11UnorderedAccessView> backbufferUAV;

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

	int numberOfLights;

	//Intensity of the light. This is the multiplied with the diffuse factor before dividing with the attenuation factor
	float lightIntensity;

	//Used to calculate light offsets (relative to each other)
	float lightSinValMult;
	float lightOtherSinValMult;

	float lightSinVal;
	float lightOtherSinVal;

	//Radius of the rotating point lights
	float lightRotationRadius;
	//Top and bottom height of all moving point lights
	float lightMinHeight;
	float lightMaxHeight;

	float lightVerticalSpeed;
	float lightHorizontalSpeed;

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

	Graph cpuGraph;

	Console console;
	bool drawConsole;

	ShaderProgram* currentShaderProgram;

#if USE_ALL_SHADER_PROGRAMS
	std::unique_ptr<ConstantBufferShaderProgram> constantBufferShaderProgram;
	std::unique_ptr<StructuredBufferShaderProgram> structuredBufferShaderProgram;
	std::unique_ptr<AABBStructuredBufferShaderProgram> aabbStructuredBufferShaderProgram;

	std::vector<ShaderProgram*> shaderPrograms;
#elif USE_CONSTANT_BUFFER_SHADER_PROGRAM
	std::unique_ptr<ConstantBufferShaderProgram> constantBufferShaderProgram;
#elif USE_STRUCTURED_BUFFER_SHADER_PROGRAM
	std::unique_ptr<StructuredBufferShaderProgram> structuredBufferShaderProgram;
#elif USE_AABBSTRUCTUREDBUFFER_SHADER_PROGRAM
	std::unique_ptr<AABBStructuredBufferShaderProgram> aabbStructuredBufferShaderProgram;
#endif

	Argument ResetCamera(const std::vector<Argument>& argument);
	Argument PauseCamera(const std::vector<Argument>& argument);
	Argument StartCamera(const std::vector<Argument>& argument);

	Argument AddCameraFrame(const std::vector<Argument>& argument);
	Argument SetCameraFrame(const std::vector<Argument>& argument);
	Argument RemoveCameraFrame(const std::vector<Argument>& argument);
	Argument PrintCameraFrames(const std::vector<Argument>& argument);
	Argument SetCameraTargetSpeed(const std::vector<Argument>& argument);

#if USE_ALL_SHADER_PROGRAMS
	Argument SetShaderProgram(const std::vector<Argument>& argument);
#endif

	bool InitSRVs();

	bool InitBulb();
	bool InitPointLights();
	bool InitGraphs();
	bool InitRoom();
	bool InitBezier();

	void InitInput();
	void InitConsole();

	void DrawUpdatePointlights();

	void DrawUpdateMVP();

	void DrawBulbs();
	void DrawBezier();

	bool CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv);
};


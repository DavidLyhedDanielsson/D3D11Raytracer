#ifndef ShaderProgram_h__
#define ShaderProgram_h__

#include <string>
#include <chrono>

#include <DXLib/Common.h>
#include <DXLib/Logger.h>
#include <DXLib/DXConstantBuffer.h>
#include <DXLib/D3D11Timer.h>

#include <DXConsole/argument.h>

#include "SharedShaderConstants.h"
#include "Graph.h"

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
	struct CameraPositionBufferData
	{
		DirectX::XMFLOAT3 position;
	};
}

struct LightAttenuation
{
	float factors[3];
};

struct PointLights
{
	DirectX::XMFLOAT4 lights[MAX_POINT_LIGHTS];
	int lightCount;
};

class Console;
class ContentManager;
class SpriteRenderer;

class ShaderProgram
{
public:
	ShaderProgram();
	virtual ~ShaderProgram() = default;

	virtual bool Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager);
	virtual bool InitBuffers(ID3D11UnorderedAccessView* backBufferUAV);

	virtual void AddOBJ(const std::string& path, DirectX::XMFLOAT3 position) = 0;
	virtual void AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color) = 0;

	virtual void Update(std::chrono::nanoseconds delta);
	virtual void Draw() = 0;
	
	void DrawGraph(SpriteRenderer* spriteRenderer);
	void DrawGraph();

	std::string ReloadShaders();

	void SetLightAttenuationFactors(LightAttenuation factors);
	void SetPointLights(PointLights pointLights);
	void SetViewProjMatrix(DirectX::XMFLOAT4X4 viewProjMatrix);
	void SetCameraPosition(DirectX::XMFLOAT3 cameraPosition);

	LightAttenuation GetLightAttenuationFactors() const;
	PointLights GetPointLights();

protected:
	std::string CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv);

	DirectX::XMFLOAT4X4 viewProjMatrix;
	DirectX::XMFLOAT3 cameraPosition;

	Console* console;
	ContentManager* contentManager;

	std::string shaderPath;

	Graph graph;
	D3D11Timer d3d11Timer;

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	ID3D11UnorderedAccessView* backBufferUAV;

	UINT backBufferWidth;
	UINT backBufferHeight;

	const static int MAX_BOUNCES = 20;
	int rayBounces;

	////////////////////
	//Point lights
	////////////////////
	DXConstantBuffer pointlightAttenuationBuffer;
	DXConstantBuffer pointLightBuffer;
	DXConstantBuffer cameraPositionBuffer;
	PointLights pointLightBufferData;
	LightAttenuation pointlightAttenuationBufferData;

	virtual bool InitUAVSRV() = 0;
	virtual bool InitShaders() = 0;

	Argument ConsoleReloadShaders(const std::vector<Argument>& argument);

	virtual std::string ReloadShadersInternal() = 0;
	bool InitPointLights();
	bool InitGraph();
};

inline bool operator>>(const LightAttenuation& lhs, Argument& rhs)
{
	rhs.values.resize(3, "");
	
	rhs.values[0] = std::to_string(lhs.factors[0]);
	rhs.values[1] = std::to_string(lhs.factors[1]);
	rhs.values[2] = std::to_string(lhs.factors[2]);

	return true;
}

inline bool operator>>(const Argument& lhs, LightAttenuation& rhs)
{
	if(lhs.values.size() != 3)
		return false;

	std::stringstream sstream;
	sstream << lhs.values[0];
	sstream >> rhs.factors[0];
	if(sstream.fail())
		return false;

	sstream.str(std::string());
	sstream << lhs.values[1];
	sstream >> rhs.factors[1];
	if(sstream.fail())
		return false;

	sstream.str(std::string());
	sstream << lhs.values[2];
	sstream >> rhs.factors[2];
	if(sstream.fail())
		return false;

	return true;
}

#endif //ShaderProgram_h__
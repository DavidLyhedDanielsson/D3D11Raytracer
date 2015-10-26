#include "ShaderProgram.h"

#include <DXConsole/console.h>
#include <DXConsole/commandGetSet.h>
#include <DXConsole/commandCallMethod.h>
#include <DXConsole/commandGetterSetter.h>

#include <DXLib/ContentManager.h>

ShaderProgram::ShaderProgram()
	: device(nullptr)
	, deviceContext(nullptr)
	, backBufferUAV(nullptr)
{
}

bool ShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	this->console = console;
	this->device = device;
	this->deviceContext = deviceContext;
	this->backBufferUAV = backBufferUAV;
	this->backBufferWidth = backBufferWidth;
	this->backBufferHeight = backBufferHeight;
	this->contentManager = contentManager;

	auto rayBouncesCommand = new CommandGetSet<int>("rayBounces", &rayBounces);
	if(!console->AddCommand(rayBouncesCommand))
		delete rayBouncesCommand;

	auto reloadShadersCommand = new CommandCallMethod("ReloadShaders", std::bind(&ShaderProgram::ConsoleReloadShaders, this, std::placeholders::_1), false);
	if(!console->AddCommand(reloadShadersCommand))
		delete reloadShadersCommand;

	if(!InitPointLights())
		return false;
	if(!InitGraph())
		return false;

	cameraPositionBuffer.Create<DirectX::XMFLOAT3>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	pointLightBufferData.lightCount = 0;
	rayBounces = 1;

	return true;
}

bool ShaderProgram::InitBuffers(ID3D11UnorderedAccessView* backBufferUAV)
{
	this->backBufferUAV = backBufferUAV;

	return true;
}

void ShaderProgram::Update(std::chrono::nanoseconds delta)
{
}

void ShaderProgram::DrawGraph(SpriteRenderer* spriteRenderer)
{
	graph.Draw(spriteRenderer);
}

void ShaderProgram::DrawGraph()
{
	graph.Draw();
}

std::string ShaderProgram::ReloadShaders()
{
	return ReloadShadersInternal();
}

void ShaderProgram::SetNumberOfLights(int numberOfLights)
{
	//TODO!
}

void ShaderProgram::SetLightAttenuationFactors(LightAttenuation factors)
{
	pointlightAttenuationBufferData = factors;

	pointlightAttenuationBuffer.Update(deviceContext, &pointlightAttenuationBufferData);
}

int ShaderProgram::GetNumberOfLights() const
{
	return pointLightBufferData.lightCount;
}

LightAttenuation ShaderProgram::GetLightAttenuationFactors() const
{
	return pointlightAttenuationBufferData;
}

void ShaderProgram::SetPointLights(PointLights pointLights)
{
	pointLightBufferData = pointLights;

	pointLightBuffer.Update(deviceContext, &pointLightBufferData);
}

PointLights ShaderProgram::GetPointLights()
{
	return pointLightBufferData;
}

void ShaderProgram::SetViewProjMatrix(DirectX::XMFLOAT4X4 viewProjMatrix)
{
	this->viewProjMatrix = viewProjMatrix;
}

void ShaderProgram::SetCameraPosition(DirectX::XMFLOAT3 cameraPosition)
{
	this->cameraPosition = cameraPosition;
}

std::string ShaderProgram::CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;

	COMUniquePtr<ID3D11Texture2D> texture(nullptr);

	ID3D11Texture2D* textureDumb = nullptr;
	texture.reset(textureDumb);
	HRESULT hRes = device->CreateTexture2D(&desc, nullptr, &textureDumb);
	if(FAILED(hRes))
		return "Couldn't create texture with dimensions " + std::to_string(width) + "x" + std::to_string(height);

	ID3D11UnorderedAccessView* uavDumb;
	hRes = device->CreateUnorderedAccessView(textureDumb, nullptr, &uavDumb);
	uav.reset(uavDumb);
	if(FAILED(hRes))
		return "Couldn't create UAV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height);

	ID3D11ShaderResourceView* srvDumb;
	hRes = device->CreateShaderResourceView(textureDumb, nullptr, &srvDumb);
	srv.reset(srvDumb);
	if(FAILED(hRes))
		return "Couldn't create SRV from texture with dimensions " + std::to_string(width) + "x" + std::to_string(height);

	return "";
}

Argument ShaderProgram::ConsoleReloadShaders(const std::vector<Argument>& argument)
{
	return ReloadShadersInternal();
}

bool ShaderProgram::InitPointLights()
{
	LogErrorReturnFalse(cameraPositionBuffer.Create<CameraPositionBufferData>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create point light camera position buffer: ");
	LogErrorReturnFalse(pointlightAttenuationBuffer.Create<LightAttenuation>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create light attenuation buffer: ");
	LogErrorReturnFalse(pointLightBuffer.Create<PointLights>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE), "Couldn't create point light buffer: ");

	auto setNumberOfLights = new CommandGetterSetter<int>("numberOfLights", std::bind(&ShaderProgram::GetNumberOfLights, this), std::bind(&ShaderProgram::SetNumberOfLights, this, std::placeholders::_1));
	if(!console->AddCommand(setNumberOfLights))
		delete setNumberOfLights;

	auto setLightAttenuationFactors = new CommandGetterSetter<LightAttenuation>("lightAttenuationFactors", std::bind(&ShaderProgram::GetLightAttenuationFactors, this), std::bind(&ShaderProgram::SetLightAttenuationFactors, this, std::placeholders::_1));
	if(!console->AddCommand(setLightAttenuationFactors))
		delete setLightAttenuationFactors;

	return true;
}

bool ShaderProgram::InitGraph()
{
#ifdef _DEBUG
	int gpuMSAverage = 10;
#else
	int gpuMSAverage = 10;
#endif

	std::vector<Track> gpuTracks{ Track(gpuMSAverage, 1.0f) };

	std::vector<std::string> timerQueries{ "Primary" }; // , "Trace", "Shade" };

	for(int i = 0; i < MAX_BOUNCES; i++)
	{
		timerQueries.emplace_back("Trace" + std::to_string(i));
		timerQueries.emplace_back("Shade" + std::to_string(i));
	}

	if(!d3d11Timer.Init(device, deviceContext, timerQueries))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize d3d11Timer");
		return false;
	}

	std::vector<std::string> gpuTrackNames{ "Primary", "Trace", "Shade" };

	std::string errorString = graph.Init(device, deviceContext, contentManager, DirectX::XMINT2(0, 720 - 128), DirectX::XMINT2(256, 128), 100.0f, 1, backBufferWidth, backBufferHeight, true);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize GPU graph: " + errorString);
		return false;
	}

	errorString = graph.AddTracks(gpuTrackNames, gpuTracks);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't add GPU tracks to graph: " + errorString);
		return false;
	}

	return true;
}

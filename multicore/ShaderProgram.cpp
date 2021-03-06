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
	, console(nullptr)
{
}

bool ShaderProgram::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager)
{
	this->console = console;
	this->device = device;
	this->deviceContext = deviceContext;
	this->depthBufferUAV = depthBufferUAV;
	this->backBufferUAV = backBufferUAV;
	this->backBufferWidth = backBufferWidth;
	this->backBufferHeight = backBufferHeight;
	this->contentManager = contentManager;

	if(!InitPointLights())
		return false;
	if(!InitTimer())
		return false;

	cameraPositionBuffer.Create<DirectX::XMFLOAT3>(device, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	pointLightBufferData.lightCount = 0;
	rayBounces = 1;

	return true;
}

bool ShaderProgram::InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV)
{
	this->depthBufferUAV = depthBufferUAV;
	this->backBufferUAV = backBufferUAV;

	return true;
}

void ShaderProgram::Update(std::chrono::nanoseconds delta)
{
}

std::string ShaderProgram::ReloadShaders()
{
	return ReloadShadersInternal();
}

void ShaderProgram::SetLightAttenuationFactors(LightAttenuation factors)
{
	pointlightAttenuationBufferData = factors;

	pointlightAttenuationBuffer.Update(deviceContext, &pointlightAttenuationBufferData);
}

void ShaderProgram::SetPointLights(PointLights pointLights)
{
	pointLightBufferData = pointLights;

	pointLightBuffer.Update(deviceContext, &pointLightBufferData);
}

void ShaderProgram::SetViewProjMatrix(DirectX::XMFLOAT4X4 viewProjMatrix)
{
	this->viewProjMatrix = viewProjMatrix;
}

void ShaderProgram::SetCameraPosition(DirectX::XMFLOAT3 cameraPosition)
{
	this->cameraPosition = cameraPosition;
}

int ShaderProgram::GetRayBounces() const
{
	return rayBounces;
}

LightAttenuation ShaderProgram::GetLightAttenuationFactors() const
{
	return pointlightAttenuationBufferData;
}

PointLights ShaderProgram::GetPointLights() const
{
	return pointLightBufferData;
}

std::string ShaderProgram::CreateUAVSRVCombo(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, COMUniquePtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT format /*= DXGI_FORMAT_R32G32B32A32_FLOAT*/)
{
	uav.reset();
	srv.reset();

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = format;
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

std::string ShaderProgram::CreateUAV(int width, int height, COMUniquePtr<ID3D11UnorderedAccessView>& uav, DXGI_FORMAT format /*= DXGI_FORMAT_R32G32B32A32_FLOAT*/)
{
	uav.reset();

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = format;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
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

	return "";
}

std::string ShaderProgram::CreateSRV(int width, int height, COMUniquePtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT format /*= DXGI_FORMAT_R32G32B32A32_FLOAT*/)
{
	srv.reset();

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = width;
	desc.Height = height;

	desc.ArraySize = 1;
	desc.Format = format;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
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

	return true;
}

bool ShaderProgram::InitTimer()
{
	std::vector<std::string> timerQueries{ "Primary" };

	for(int i = 0; i < MAX_BOUNCES; i++)
	{
		timerQueries.emplace_back("Intersect" + std::to_string(i));
		timerQueries.emplace_back("Shade" + std::to_string(i));
	}

	if(!d3d11Timer.Init(device, deviceContext, timerQueries))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't initialize d3d11Timer");
		return false;
	}

	return true;
}

void ShaderProgram::SetRayBounces(int bounces)
{
	if(bounces > MAX_BOUNCES)
	{
		bounces = MAX_BOUNCES;
		Logger::LogLine(LOG_TYPE::WARNING, "Can't set bounces to a value larger than " + std::to_string(MAX_BOUNCES) + "(MAX_BOUNCES)");
	}

	rayBounces = bounces;
}

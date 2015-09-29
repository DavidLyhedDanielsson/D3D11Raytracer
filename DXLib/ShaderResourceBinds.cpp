#include "ShaderResourceBinds.h"

#include <map>

ShaderResourceBinds::ShaderResourceBinds()
	: cbufferCount(0)
	, samplerCount(0)
	, uavCount(0)
	, srvCount(0)
	, unknownBuffers(0)
{}

void ShaderResourceBinds::Init(ID3D11Device* device, ID3D11ShaderReflection* reflection, std::string shaderPath)
{
	D3D11_SHADER_DESC desc;
	reflection->GetDesc(&desc);

	UINT boundResources = desc.BoundResources;
	for(UINT i = 0; i < boundResources; i++)
	{
		D3D11_SHADER_INPUT_BIND_DESC inputDesc;
		reflection->GetResourceBindingDesc(i, &inputDesc);

		switch(inputDesc.Type)
		{
			case D3D_SIT_CBUFFER:
				++cbufferCount;
				break;
			case D3D_SIT_SAMPLER:
				++samplerCount;
				break;
			case D3D_SIT_UAV_RWTYPED:
				++uavCount;
				break;
			case D3D_SIT_TEXTURE:
				++srvCount;
				break;
			default:
				unknownBuffers.emplace_back(inputDesc.Name);
				break;
		}
	}

	cbuffersNullptr.resize(cbufferCount, nullptr);
	samplersNullptr.resize(samplerCount, nullptr);
	uavsNullptr.resize(uavCount, nullptr);
	srvsNullptr.resize(srvCount, nullptr);

	if(cbuffers.size() > cbufferCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added more buffers than expected to \"" + shaderPath + "\"");
	else if(cbuffers.size() < cbufferCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added fewer buffers than expected to \"" + shaderPath + "\"");

	if(samplers.size() > samplerCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added more samplers than expected to \"" + shaderPath + "\"");
	else if(samplers.size() < samplerCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added fewer samplers than expected to \"" + shaderPath + "\"");

	if(uavs.size() > uavCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added more UAVs than expected to \"" + shaderPath + "\"");
	else if(uavs.size() < uavCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added fewer UAVs than expected to \"" + shaderPath + "\"");

	if(srvs.size() > srvCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added more SRVs than expected to \"" + shaderPath + "\"");
	else if(srvs.size() < srvCount)
		Logger::LogLine(LOG_TYPE::WARNING, "Added fewer SRVs than expected to \"" + shaderPath + "\"");
}

void ShaderResourceBinds::AddResource(ID3D11Buffer* buffer)
{
	cbuffers.push_back(buffer);
}

void ShaderResourceBinds::AddResource(ID3D11SamplerState* sampler)
{
	samplers.push_back(sampler);
}

void ShaderResourceBinds::AddResource(ID3D11UnorderedAccessView* uav)
{
	uavs.push_back(uav);
}

void ShaderResourceBinds::AddResource(ID3D11ShaderResourceView* srv)
{
	srvs.push_back(srv);
}

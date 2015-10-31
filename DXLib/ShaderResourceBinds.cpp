#include "ShaderResourceBinds.h"

#include "DXStructuredBuffer.h"

#include <unordered_map>

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

	cbufferCount = static_cast<UINT>(cbuffers.size());
	samplerCount = static_cast<UINT>(samplers.size());
	uavCount = static_cast<UINT>(uavs.size());
	srvCount = static_cast<UINT>(srvs.size());

	/*if(cbuffers.size() > 0)
		cbuffersNullptr.resize(cbuffers.size(), nullptr);
	if(samplers.size() > 0)
		samplersNullptr.resize(samplers.size(), nullptr);
	if(uavs.size() > 0)
		uavsNullptr.resize(uavs.size(), nullptr);
	if(srvs.size() > 0)
		srvsNullptr.resize(srvs.size(), nullptr);*/

	ConcatMap(cbuffers);
	ConcatMap(samplers);
	ConcatMap(uavs);
	ConcatMap(srvs);

	cbuffersNullptr = cbuffers;
	for(auto& pair : cbuffersNullptr)
		std::fill(pair.second.begin(), pair.second.end(), nullptr);

	samplersNullptr = samplers;
	for(auto& pair : samplersNullptr)
		std::fill(pair.second.begin(), pair.second.end(), nullptr);

	uavsNullptr = uavs;
	for(auto& pair : uavsNullptr)
		std::fill(pair.second.begin(), pair.second.end(), nullptr);

	srvsNullptr = srvs;
	for(auto& pair : srvsNullptr)
		std::fill(pair.second.begin(), pair.second.end(), nullptr);

	std::set<UINT> cbufferRegisters;
	std::set<UINT> samplerRegisters;
	std::set<UINT> uavRegisters;
	std::set<UINT> srvRegisters;

	UINT boundResources = desc.BoundResources;
	for(UINT i = 0; i < boundResources; i++)
	{
		D3D11_SHADER_INPUT_BIND_DESC inputDesc;
		reflection->GetResourceBindingDesc(i, &inputDesc);

		switch(inputDesc.Type)
		{
			case D3D_SIT_CBUFFER:
				cbufferRegisters.insert(inputDesc.BindPoint);
				break;
			case D3D_SIT_SAMPLER:
				samplerRegisters.insert(inputDesc.BindPoint);
				break;
			case D3D_SIT_UAV_RWTYPED:
				uavRegisters.insert(inputDesc.BindPoint);
				break;
			case D3D_SIT_TEXTURE:
				srvRegisters.insert(inputDesc.BindPoint);
				break;
			case D3D_SIT_STRUCTURED: //TODO: UAV?
				srvRegisters.insert(inputDesc.BindPoint);
				break;
			default:
				unknownBuffers.emplace_back(inputDesc.Name);
				break;
		}
	}

	CheckBoundSlots(cbuffers, cbufferRegisters, "cbuffer", shaderPath);
	CheckBoundSlots(samplers, samplerRegisters, "sampler", shaderPath);
	CheckBoundSlots(uavs, uavRegisters, "UAV", shaderPath);
	CheckBoundSlots(srvs, srvRegisters, "SRV", shaderPath);

	for(const auto& uavPair : uavs)
	{
		for(int i = 0, endI = static_cast<int>(uavPair.second.size()); i < endI; ++i)
		{
			int iOffset = uavPair.first;

			if(uavPair.second[i] == nullptr)
				continue;;

			ID3D11Resource* uavResource = nullptr;
			uavPair.second[i]->GetResource(&uavResource);

			for(const auto& srvPair : srvs)
			{
				for(int j = 0, endJ = static_cast<int>(srvPair.second.size()); j < endJ; ++j)
				{
					int jOffset = srvPair.first;

					if(srvPair.second[j] == nullptr) //TODO: Warning?
						continue;;

					ID3D11Resource* srvResource = nullptr;
					srvPair.second[j]->GetResource(&srvResource);

					if(uavResource == srvResource)
						Logger::LogLine(LOG_TYPE::WARNING, "Binding the same resource to u" + std::to_string(i + iOffset) + " and t" + std::to_string(j + jOffset) + "\" in \"" + shaderPath + "\"");
				}
			}
		}
	}
}

void ShaderResourceBinds::AddResource(const DXConstantBuffer& buffer, int slot)
{
	if(cbuffers.find(slot) != cbuffers.end())
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to bind multiple cbuffers to slot " + std::to_string(slot));

	cbuffers[slot].push_back(buffer.GetBuffer());
}

void ShaderResourceBinds::AddResource(const DXStructuredBuffer& buffer, int slot)
{
	AddResource(buffer.GetSRV(), slot);
}

void ShaderResourceBinds::AddResource(ID3D11SamplerState* sampler, int slot)
{
	if(samplers.find(slot) != samplers.end())
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to bind multiple samplers to slot " + std::to_string(slot));

	samplers[slot].push_back(sampler);
}

void ShaderResourceBinds::AddResource(ID3D11UnorderedAccessView* uav, int slot)
{
	if(uavs.find(slot) != uavs.end())
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to bind multiple UAVs to slot " + std::to_string(slot));

	uavs[slot].push_back(uav);
}

void ShaderResourceBinds::AddResource(ID3D11ShaderResourceView* srv, int slot)
{
	if(srvs.find(slot) != srvs.end())
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to bind multiple SRVs to slot " + std::to_string(slot));

	srvs[slot].push_back(srv);
}

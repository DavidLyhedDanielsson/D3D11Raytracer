#ifndef ShaderResourceBinds_h__
#define ShaderResourceBinds_h__

#include "Common.h"
#include "Logger.h"
#include "DXConstantBuffer.h"

#include <vector>
#include <string>
#include <map>
#include <set>

#include <d3d11shader.h>

class ComputeShader;
class PixelShader;
class VertexShader;
class HullShader;
class DomainShader;
class DXStructuredBuffer;

class ShaderResourceBinds
{
public:
	ShaderResourceBinds();
	~ShaderResourceBinds() = default;

	void Init(ID3D11Device* device, ID3D11ShaderReflection* reflection, std::string shaderPath);

	void AddResource(const DXConstantBuffer& buffer, int slot);
	void AddResource(const DXStructuredBuffer& buffer, int slot);
	void AddResource(ID3D11SamplerState* sampler, int slot);
	void AddResource(ID3D11UnorderedAccessView* uav, int slot);
	void AddResource(ID3D11ShaderResourceView* srv, int slot);

	template<typename T>
	void Bind(ID3D11DeviceContext* deviceContext)
	{}

	template<typename T>
	void Unbind(ID3D11DeviceContext* deviceContext)
	{}

	template<>
	void Bind<ComputeShader>(ID3D11DeviceContext* deviceContext)
	{
		for(const auto& pair : cbuffers)
			deviceContext->CSSetConstantBuffers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : samplers)
			deviceContext->CSSetSamplers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : uavs)
			deviceContext->CSSetUnorderedAccessViews(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0], nullptr);
		for(const auto& pair : srvs)
			deviceContext->CSSetShaderResources(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
	}

	template<>
	void Bind<PixelShader>(ID3D11DeviceContext* deviceContext)
	{
		for(const auto& pair : cbuffers)
			deviceContext->PSSetConstantBuffers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : samplers)
			deviceContext->PSSetSamplers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : srvs)
			deviceContext->PSSetShaderResources(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
	}

	template<>
	void Bind<VertexShader>(ID3D11DeviceContext* deviceContext)
	{
		for(const auto& pair : cbuffers)
			deviceContext->VSSetConstantBuffers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : samplers)
			deviceContext->VSSetSamplers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : srvs)
			deviceContext->VSSetShaderResources(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
	}

	template<>
	void Bind<HullShader>(ID3D11DeviceContext* deviceContext)
	{
		for(const auto& pair : cbuffers)
			deviceContext->HSSetConstantBuffers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : samplers)
			deviceContext->HSSetSamplers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : srvs)
			deviceContext->HSSetShaderResources(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
	}

	template<>
	void Bind<DomainShader>(ID3D11DeviceContext* deviceContext)
	{
		for(const auto& pair : cbuffers)
			deviceContext->DSSetConstantBuffers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : samplers)
			deviceContext->DSSetSamplers(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
		for(const auto& pair : srvs)
			deviceContext->DSSetShaderResources(pair.first, static_cast<UINT>(pair.second.size()), &pair.second[0]);
	}

	template<>
	void Unbind<ComputeShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->CSSetConstantBuffers(0, cbufferCount, &cbuffersNullptr[0]);
		if(samplerCount > 0)
			deviceContext->CSSetSamplers(0, samplerCount, &samplersNullptr[0]);
		if(uavCount > 0)
			deviceContext->CSSetUnorderedAccessViews(0, uavCount, &uavsNullptr[0], nullptr);
		if(srvCount > 0)
			deviceContext->CSSetShaderResources(0, srvCount, &srvsNullptr[0]);
	}

	template<>
	void Unbind<PixelShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->PSSetConstantBuffers(0, cbufferCount, &cbuffersNullptr[0]);
		if(samplerCount > 0)
			deviceContext->PSSetSamplers(0, samplerCount, &samplersNullptr[0]);
		if(srvCount > 0)
			deviceContext->PSSetShaderResources(0, srvCount, &srvsNullptr[0]);
	}

	template<>
	void Unbind<VertexShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->PSSetConstantBuffers(0, cbufferCount, &cbuffersNullptr[0]);
		if(samplerCount > 0)
			deviceContext->PSSetSamplers(0, samplerCount, &samplersNullptr[0]);
		if(srvCount > 0)
			deviceContext->PSSetShaderResources(0, srvCount, &srvsNullptr[0]);
	}

	template<>
	void Unbind<HullShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->HSSetConstantBuffers(0, cbufferCount, &cbuffersNullptr[0]);
		if(samplerCount > 0)
			deviceContext->HSSetSamplers(0, samplerCount, &samplersNullptr[0]);
		if(srvCount > 0)
			deviceContext->HSSetShaderResources(0, srvCount, &srvsNullptr[0]);
	}

	template<>
	void Unbind<DomainShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->DSSetConstantBuffers(0, cbufferCount, &cbuffersNullptr[0]);
		if(samplerCount > 0)
			deviceContext->DSSetSamplers(0, samplerCount, &samplersNullptr[0]);
		if(srvCount > 0)
			deviceContext->DSSetShaderResources(0, srvCount, &srvsNullptr[0]);
	}
private:
	UINT cbufferCount;
	UINT samplerCount;
	UINT uavCount;
	UINT srvCount;

	std::vector<std::string> unknownBuffers;

	//<startSlot, buffers>
	std::map<int, std::vector<ID3D11Buffer*>> cbuffers;
	std::map<int, std::vector<ID3D11SamplerState*>> samplers;
	std::map<int, std::vector<ID3D11UnorderedAccessView*>> uavs;
	std::map<int, std::vector<ID3D11ShaderResourceView*>> srvs;

	//There's probably a better way of setting nullptrs
	std::vector<ID3D11Buffer*> cbuffersNullptr;
	std::vector<ID3D11SamplerState*> samplersNullptr;
	std::vector<ID3D11UnorderedAccessView*> uavsNullptr;
	std::vector<ID3D11ShaderResourceView*> srvsNullptr;

	template<typename T>
	void ConcatMap(std::map<int, std::vector<T>>& lhs)
	{
		if(lhs.size() > 0)
		{
			for(auto iter = --lhs.end(), end = lhs.begin(); iter != end;)
			{
				auto previous = iter;
				--previous;

				if(previous->first + static_cast<int>(previous->second.size()) == iter->first)
				{
					//Append this vector to the one in the previous slot
					previous->second.insert(previous->second.end(), iter->second.begin(), iter->second.end());

					lhs.erase(iter);
					iter = previous;
				}
				else
					--iter;
			}
		}
	}

	template<typename T>
	void CheckBoundSlots(std::map<int, std::vector<T>>& shaderResources, std::set<UINT> registers, const std::string& type, const std::string& path)
	{
		for(const auto& pair : shaderResources)
		{
			for(int i = pair.first, end = pair.first + static_cast<UINT>(pair.second.size()); i < end; ++i)
			{
				if(registers.count(i) > 0)
					registers.erase(i);
				else
					Logger::LogLine(LOG_TYPE::INFO, "No " + type + " expected on slot " + std::to_string(i) + " in shader \"" + path + "\" (is it optimized away?)");
			}
		}

		for(UINT bindPoint : registers)
			Logger::LogLine(LOG_TYPE::INFO, "No " + type + " bound to slot " + std::to_string(bindPoint) + " in shader \"" + path + "\"");
	}
};

#endif // ShaderResourceBinds_h__

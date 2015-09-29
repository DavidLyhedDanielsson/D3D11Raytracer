#ifndef ShaderResourceBinds_h__
#define ShaderResourceBinds_h__

#include "Common.h"
#include "Logger.h"

#include <vector>
#include <string>

#include <d3d11shader.h>

class ComputeShader;
class PixelShader;
class VertexShader;

class ShaderResourceBinds
{
public:
	ShaderResourceBinds();
	~ShaderResourceBinds() = default;

	void Init(ID3D11Device* device, ID3D11ShaderReflection* reflection, std::string shaderPath);

	void AddResource(ID3D11Buffer* buffer);
	void AddResource(ID3D11SamplerState* sampler);
	void AddResource(ID3D11UnorderedAccessView* uav);
	void AddResource(ID3D11ShaderResourceView* srv);

	template<typename T>
	void Bind(ID3D11DeviceContext* deviceContext)
	{}

	template<typename T>
	void Unbind(ID3D11DeviceContext* deviceContext)
	{}

	template<>
	void Bind<ComputeShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->CSSetConstantBuffers(0, cbufferCount, &cbuffers[0]);
		if(samplerCount > 0)
			deviceContext->CSSetSamplers(0, samplerCount, &samplers[0]);
		if(uavCount > 0)
			deviceContext->CSSetUnorderedAccessViews(0, uavCount, &uavs[0], nullptr);
		if(srvCount > 0)
			deviceContext->CSSetShaderResources(0, srvCount, &srvs[0]);
	}

	template<>
	void Bind<PixelShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->PSSetConstantBuffers(0, cbufferCount, &cbuffers[0]);
		if(samplerCount > 0)
			deviceContext->PSSetSamplers(0, samplerCount, &samplers[0]);
		if(srvCount > 0)
			deviceContext->PSSetShaderResources(0, srvCount, &srvs[0]);
	}

	template<>
	void Bind<VertexShader>(ID3D11DeviceContext* deviceContext)
	{
		if(cbufferCount > 0)
			deviceContext->VSSetConstantBuffers(0, cbufferCount, &cbuffers[0]);
		if(samplerCount > 0)
			deviceContext->VSSetSamplers(0, samplerCount, &samplers[0]);
		if(srvCount > 0)
			deviceContext->VSSetShaderResources(0, srvCount, &srvs[0]);
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
private:
	UINT cbufferCount;
	UINT samplerCount;
	UINT uavCount;
	UINT srvCount;

	std::vector<std::string> unknownBuffers;

	std::vector<ID3D11Buffer*> cbuffers;
	std::vector<ID3D11Buffer*> cbuffersNullptr; //There's probably a better way of doing this
	std::vector<ID3D11SamplerState*> samplers;
	std::vector<ID3D11SamplerState*> samplersNullptr;
	std::vector<ID3D11UnorderedAccessView*> uavs;
	std::vector<ID3D11UnorderedAccessView*> uavsNullptr;
	std::vector<ID3D11ShaderResourceView*> srvs;
	std::vector<ID3D11ShaderResourceView*> srvsNullptr;
};

#endif // ShaderResourceBinds_h__

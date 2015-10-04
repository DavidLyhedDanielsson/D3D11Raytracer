#ifndef Shader_h__
#define Shader_h__

#include "common.h"

#include "ShaderResourceBinds.h"

#include <d3dcompiler.h>

#include <string>

template<typename T>
class Shader
{
public:
	Shader(const std::string& entryPoint, const std::string& compileVersion)
		: shader(nullptr)
		, shaderBlob(nullptr)
		, entryPoint(entryPoint)
		, compileVersion(compileVersion)
	{

	}
	virtual ~Shader() = default;

	std::string Recreate(ID3D11Device* device)
	{
		std::string returnMessage;

		if(shaderPath.find(".cso") == shaderPath.size() - 4)
			returnMessage = LoadPrecompiledShader(shaderPath, device);
		else
			returnMessage = LoadAndCompileShader(shaderPath, device);

		return returnMessage;
	}

	std::string CreateFromFile(const std::string& path, ID3D11Device* const device)
	{
		shaderPath = path;
		std::string returnMessage;

		if(path.find(".cso") == path.size() - 4)
			returnMessage = LoadPrecompiledShader(path, device);
		else
			returnMessage = LoadAndCompileShader(path, device);

		if(!returnMessage.empty())
			return returnMessage;

		ID3D11ShaderReflection* reflectionDumb = nullptr;
		D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&reflectionDumb));
		COMUniquePtr<ID3D11ShaderReflection> reflection(reflectionDumb);

		return "";
	}

	template<typename... ResourceBinds>
	std::string CreateFromFile(const std::string& path, ID3D11Device* const device, ResourceBinds... resourceBinds)
	{
		shaderPath = path;
		std::string returnMessage;

		if(path.find(".cso") == path.size() - 4)
			returnMessage = LoadPrecompiledShader(path, device);
		else
			returnMessage = LoadAndCompileShader(path, device);

		if(!returnMessage.empty())
			return returnMessage;

		this->resourceBinds = { resourceBinds... };

		ID3D11ShaderReflection* reflectionDumb = nullptr;
		D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&reflectionDumb));
		COMUniquePtr<ID3D11ShaderReflection> reflection(reflectionDumb);

		for(int i = 0; i < static_cast<int>(this->resourceBinds.size()); ++i)
			this->resourceBinds[i].Init(device, reflectionDumb, shaderPath);

		return "";
	}

	virtual void Bind(ID3D11DeviceContext* context) = 0;
	virtual void Unbind(ID3D11DeviceContext* context) = 0;

	template<typename T>
	void BindResources(ID3D11DeviceContext* context, int configuration = 0)
	{
		boundConfiguration = configuration;

		if(resourceBinds.size() > 0)
			resourceBinds[boundConfiguration].Bind<T>(context);
	}
	template<typename T>
	void UnbindResources(ID3D11DeviceContext* context)
	{
		if(resourceBinds.size() > 0)
			resourceBinds[boundConfiguration].Unbind<T>(context);
	}

	ID3DBlob* GetBlob()
	{
		return shaderBlob.get();
	}
	std::string GetPath()
	{
		return shaderPath;
	}

protected:
	COMUniquePtr<T> shader;
	COMUniquePtr<ID3DBlob> shaderBlob;

	std::string shaderPath;

	std::vector<ShaderResourceBinds> resourceBinds;
	int boundConfiguration;

private:
	std::string entryPoint;
	std::string compileVersion;

	std::string LoadPrecompiledShader(const std::string& path, ID3D11Device* const device)
	{
		ID3DBlob* shaderBlobDumb = nullptr;

		std::wstring widePath = std::wstring(path.begin(), path.end());
		HRESULT hRes = D3DReadFileToBlob(widePath.c_str(), &shaderBlobDumb);
		shaderBlob.reset(shaderBlobDumb);

		if(FAILED(hRes))
			return "Couldn't load shader at \"" + path + "\"";

		if(!CreateShader(shaderBlob.get(), device))
			return "Couldn't create compute shader from file at \"" + path + "\"";

		return "";
	}
	std::string LoadAndCompileShader(const std::string& path, ID3D11Device* const device)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
		flags |= D3DCOMPILE_DEBUG;
		flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#else
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		std::wstring wPath(path.begin(), path.end());

		ID3DBlob* shaderBlobDumb = nullptr;
		ID3DBlob* errorBlobDumb = nullptr;

		HRESULT hRes = D3DCompileFromFile(wPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), compileVersion.c_str(), flags, 0, &shaderBlobDumb, &errorBlobDumb);

		shaderBlob.reset(shaderBlobDumb);
		COMUniquePtr<ID3DBlob> errorBlob(errorBlobDumb);

		if(FAILED(hRes))
		{
			if(errorBlobDumb != nullptr)
				return "Error when compiling shader at \"" + path + "\": " + reinterpret_cast<char*>(errorBlobDumb->GetBufferPointer());
			else
				return "Error when compiling shader at \"" + path + "\", but no error message was available";
		}

		if(!CreateShader(shaderBlob.get(), device))
			return "Couldn't create compute shader from file at \"" + path + "\"";

		return "";
	}
	virtual bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) = 0;
};

#endif // Shader_h__

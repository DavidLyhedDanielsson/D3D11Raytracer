#pragma once

#include "stdafx.h"

#include <string>

class ComputeShader
{
public:
	ComputeShader();
	~ComputeShader();

	std::string CreateFromFile(const std::string& path, ID3D11Device* const device);

	void Bind(ID3D11DeviceContext* context);
	void Unbind(ID3D11DeviceContext* context);

private:
	std::string LoadPrecompiledShader(const std::string& path, ID3D11Device* const device);
	std::string LoadAndCompileShader(const std::string& path, ID3D11Device* const device);
	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device);

	COMUniquePtr<ID3D11ComputeShader> shader;
};


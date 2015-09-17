#pragma once

#include "Shader.h"

class ComputeShader
	: public Shader<ID3D11ComputeShader>
{
public:
	ComputeShader(const std::string& entryPoint, const std::string& compileVersion);
	~ComputeShader();

	void Bind(ID3D11DeviceContext* context) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;
};


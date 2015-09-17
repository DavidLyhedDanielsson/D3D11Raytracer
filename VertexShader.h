#pragma once

#include "Shader.h"

class VertexShader 
	: public Shader<ID3D11VertexShader>
{
public:
	VertexShader(const std::string& entryPoint, const std::string& compileVersion);
	~VertexShader();

	void Bind(ID3D11DeviceContext* context) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;

private:
	COMUniquePtr<ID3DBlob> sourceBlob;
	COMUniquePtr<ID3D11InputLayout> inputLayout;

	std::string PostLoad(ID3D11Device* const device) override;
};


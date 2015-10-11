#ifndef DomainShader_h__
#define DomainShader_h__

#include "Shader.h"

class DomainShader :
	public Shader<ID3D11DomainShader>
{
public:
	DomainShader(const std::string& entryPoint, const std::string& compileVersion);
	~DomainShader();

	void Bind(ID3D11DeviceContext* context, int config = 0) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;
};

#endif // DomainShader_h__

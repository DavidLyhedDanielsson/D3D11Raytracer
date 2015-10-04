#ifndef PixelShader_h__
#define PixelShader_h__

#include "Shader.h"

class PixelShader 
	: public Shader<ID3D11PixelShader>
{
public:
	PixelShader(const std::string& entryPoint, const std::string& compileVersion);
	~PixelShader();

	void Bind(ID3D11DeviceContext* context, int config = 0) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;
};

#endif // PixelShader_h__

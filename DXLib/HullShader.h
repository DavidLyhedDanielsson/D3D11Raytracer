#ifndef HullShader_h__
#define HullShader_h__

#include "Shader.h"

class HullShader :
	public Shader<ID3D11HullShader>
{
public:
	HullShader(const std::string& entryPoint, const std::string& compileVersion);
	~HullShader();

	void Bind(ID3D11DeviceContext* context, int config = 0) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;
};

#endif // HullShader_h__

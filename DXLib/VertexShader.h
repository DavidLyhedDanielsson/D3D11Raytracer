#ifndef VertexShader_h__
#define VertexShader_h__

#include "Shader.h"

#include <vector>

enum class VERTEX_INPUT_DATA
{
	FLOAT
	, FLOAT2
	, FLOAT3
	, FLOAT4
};

class VertexShader 
	: public Shader<ID3D11VertexShader>
{
public:
	VertexShader(const std::string& entryPoint, const std::string& compileVersion);
	~VertexShader();

	void Bind(ID3D11DeviceContext* context) override;
	void Unbind(ID3D11DeviceContext* context) override;

	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) override;
	bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device, std::vector<VERTEX_INPUT_DATA> inputData, std::vector<std::string> inputDataNames);

	std::string SetVertexData(ID3D11Device* const device, std::vector<VERTEX_INPUT_DATA> inputData, std::vector<std::string> inputDataNames);

private:
	COMUniquePtr<ID3D11InputLayout> inputLayout;
};

#endif // VertexShader_h__

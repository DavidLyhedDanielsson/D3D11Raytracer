#ifndef AABBShaderProgram_h__
#define AABBShaderProgram_h__

#include "ShaderProgram.h"
#include "ComputeShader.h"

#include <DXLib/DXStructuredBuffer.h>

#include "Shaders/AABBStructuredBuffer/AABBStructuredBufferSharedBuffers.h"

class OBJFile;

class AABBStructuredBufferShaderProgram
	: public ShaderProgram
{
public:
	AABBStructuredBufferShaderProgram();
	~AABBStructuredBufferShaderProgram() = default;

	bool Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager) override;
	bool InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV) override;

	void Update(std::chrono::nanoseconds delta) override;
	std::map<std::string, double> Draw() override;

	void AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color) override;
	void AddOBJ(const std::string& path, DirectX::XMFLOAT3 position, float scale) override;
private:
	int dispatchX;
	int dispatchY;

	COMUniquePtr<ID3D11UnorderedAccessView> outputColorUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> outputColorSRV[2];

	//Contains the ray's current color (accumulates over bounces)
	COMUniquePtr<ID3D11UnorderedAccessView> rayColorUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayColorSRV;
	//Contains the ray's current direction (changes with every bounce)
	COMUniquePtr<ID3D11UnorderedAccessView> rayDirectionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayDirectionSRV[2];
	//Contains the ray's current position (changes with every bounce unless normal == 0)
	COMUniquePtr<ID3D11UnorderedAccessView> rayPositionUAV[2];
	COMUniquePtr<ID3D11ShaderResourceView> rayPositionSRV[2];

	//Contains the normal at the ray's intersection location
	//Also used to check if the ray didn't hit anything, in that case it will be (0, 0, 0)
	COMUniquePtr<ID3D11UnorderedAccessView> rayNormalUAV;
	COMUniquePtr<ID3D11ShaderResourceView> rayNormalSRV;

	//Used to create primary rays
	DXConstantBuffer viewProjInverseBuffer;

	//Spheres to ray trace against
	DXStructuredBuffer sphereBuffer;
	std::vector<AABBStructuredBufferSharedBuffers::Sphere> sphereBufferData;

	//////////////////////////////////////////////////
	//Triangles
	//////////////////////////////////////////////////
	std::vector<AABBStructuredBufferSharedBuffers::Vertex> vertexBufferData;
	DXStructuredBuffer triangleVertexBuffer;
	std::vector<AABBStructuredBufferSharedBuffers::Triangle> triangleBufferData;
	DXStructuredBuffer triangleBuffer;

	std::vector<AABBStructuredBufferSharedBuffers::Model> modelsBufferData;
	DXStructuredBuffer modelsBuffer;

	////////////////////
	//Shaders
	////////////////////
	ComputeShader primaryRayGenerator;
	ComputeShader traceShader;
	ComputeShader intersectionShader;
	ComputeShader compositShader;

	OBJFile* objFile;

	bool InitUAVSRV() override;
	bool InitShaders() override;

	std::string ReloadShadersInternal() override;

	void DrawRayPrimary();
	void DrawRayIntersection(int config);
	void DrawRayShading(int config);
	void DrawComposit(int config);
};

#endif //AABBShaderProgram_h__
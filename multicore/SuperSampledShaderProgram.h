#ifndef SuperSampledhaderProgram_h__
#define SuperSampledhaderProgram_h__

#include "ShaderProgram.h"
#include "ComputeShader.h"

#include <DXLib/DXStructuredBuffer.h>

#include "Shaders/SuperSampled/SuperSampledSharedBuffers.h"
#include "Shaders/Picking/PickingSharedBuffers.h"

class OBJFile;

namespace
{
	struct TextureSet
	{
		TextureSet()
			: diffuse(nullptr)
			, normal(nullptr)
		{}

		Texture2D* diffuse;
		Texture2D* normal;

		bool operator<(const TextureSet& rhs) const
		{
			return diffuse < rhs.diffuse;
		}
	};
}

class SuperSampledShaderProgram
	: public ShaderProgram
{
public:
	SuperSampledShaderProgram();
	~SuperSampledShaderProgram() = default;

	bool Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager) override;
	bool Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, UINT backBufferWidth, UINT backBufferHeight, Console* console, ContentManager* contentManager, UINT superSampleCount);
	bool InitBuffers(ID3D11UnorderedAccessView* depthBufferUAV, ID3D11UnorderedAccessView* backBufferUAV) override;

	void Update(std::chrono::nanoseconds delta) override;
	std::map<std::string, double> Draw() override;

	void AddSphere(DirectX::XMFLOAT4 sphere, DirectX::XMFLOAT4 color) override;
	void AddOBJ(const std::string& path, DirectX::XMFLOAT3 position, float scale) override;

	void Pick(const DirectX::XMINT2& mousePosition, std::function<void(const PickedObjectData&)> callback) override;

	void SetSuperSampleCount(UINT count);
	UINT GetSuperSampleCount() const;
private:
	int dispatchX;
	int dispatchY;

	UINT superSampleCount;
	UINT superSampleWidth;
	UINT superSampleHeight;

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

	COMUniquePtr<ID3D11UnorderedAccessView> depthBufferUAVUpscaled;
	COMUniquePtr<ID3D11ShaderResourceView> depthBufferSRVUpscaled;

	//Used to create primary rays
	DXConstantBuffer viewProjInverseBuffer;

	//Spheres to ray trace against
	DXStructuredBuffer sphereBuffer;
	std::vector<SuperSampledSharedBuffers::Sphere> sphereBufferData;

	//////////////////////////////////////////////////
	//Triangles
	//////////////////////////////////////////////////
	std::vector<SuperSampledSharedBuffers::Vertex> vertexBufferData;
	DXStructuredBuffer triangleVertexBuffer;
	std::vector<SuperSampledSharedBuffers::Triangle> triangleBufferData;
	DXStructuredBuffer triangleBuffer;

	std::vector<SuperSampledSharedBuffers::Model> modelsBufferData;
	DXStructuredBuffer modelsBuffer;

	////////////////////
	//Shaders
	////////////////////
	ComputeShader primaryRayGenerator;
	ComputeShader traceShader;
	ComputeShader intersectionShader;
	ComputeShader compositShader;

	DXConstantBuffer superSampleBuffer;

	OBJFile* objFile;

	//////////////////////////////////////////////////
	//Picking
	//////////////////////////////////////////////////
	ComputeShader pickingShader;

	DXConstantBuffer pickingMousePositionBuffer;
	DXStructuredBuffer pickingHitDataBuffer;

	DirectX::XMINT2 pickPosition;

	std::map<TextureSet, int> textureSets;

	bool InitUAVSRV() override;
	bool InitShaders() override;

	std::string ReloadShadersInternal() override;

	void DrawRayPrimary();
	void DrawRayIntersection(int config);
	void DrawRayShading(int config);
	void DrawComposit(int config);
	void DrawPick();
};

#endif //SuperSampledhaderProgram_h__
#ifndef Graph_h__
#define Graph_h__

#include <DXLib/Common.h>
#include <DXLib/SpriteRenderer.h>

#include <vector>
#include <string>
#include <map>

namespace
{
	struct LineVertex 
	{
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT4 color;
		float dashValue;
		uint8_t padding[4];

		LineVertex()
			: position(0.0f, 0.0f)
			, color(1.0f, 1.0f, 1.0f, 1.0f)
			, dashValue(0)
		{}
		LineVertex(DirectX::XMFLOAT2 position, DirectX::XMFLOAT4 color, float dashValue)
			: position(position)
			, color(color)
			, dashValue(dashValue)
		{}
		~LineVertex() = default;
	};
}

class Graph
{
public:
	Graph();
	~Graph();

	std::string Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height, float yMin, float ymax, int avgPoints, const std::vector<std::string>& tracks);

	bool AddTracks(const std::vector<std::string>& tracks);
	void AddValueToTrack(const std::string& track, float value);

	void Draw(SpriteRenderer* spriteRenderer);
	void Draw();

private:
	//TODO: non-static
	const static int X_SCREEN_SIZE = 1280;
	const static int Y_SCREEN_SIZE = 720;

	int width;
	int height;

	float yMin;
	float yMax;

	int avgPoints;

	DirectX::XMFLOAT2 position;

	std::map<std::string, std::vector<float>> tracks;

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;

	COMUniquePtr<ID3D11Buffer> backgroundVertexBuffer;
	COMUniquePtr<ID3D11Buffer> backgroundIndexBuffer;

	COMUniquePtr<ID3D11DepthStencilState> depthStencilState;
	COMUniquePtr<ID3D11BlendState> blendState;

	int backgroundIndicies;

	VertexShader vertexShader;
	PixelShader pixelShader;
	
	ID3D11Buffer* CreateVertexBuffer(int points, int tracks) const;
	ID3D11Buffer* CreateIndexBuffer(int points, int tracks) const;
	ID3D11Buffer* CreateBuffer(UINT size, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/);

	bool GenerateBackgroundBuffers(const DirectX::XMFLOAT2& position, int width, int height, float yMin, float yMax);
	float CalculateYPosition(float value) const;

};
#endif // Graph_h__

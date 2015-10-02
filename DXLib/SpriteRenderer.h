#ifndef SpriteRenderer_h__
#define SpriteRenderer_h__

#include <string>
#include <vector>

#include "rect.h"
#include "texture2D.h"
#include "contentManager.h"
#include "vertex2D.h"

#include "characterSet.h"
#include "spriteBatch.h"
#include "texture2DCreateParameters.h"

#include "VertexShader.h"
#include "PixelShader.h"

class SpriteRenderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	void Init(ID3D11Device* device, ID3D11DeviceContext* context, ContentManager* contentManager, int xRes, int yRes);

	void Begin();
	void End();

	//void DrawString(const Texture2D& texture2D, const DirectX::XMFLOAT2& position, const ConstructedString& characters, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), unsigned int maxRowLength = 0xFFFFFFFF);

	//TODO: Manual clipping!
	void DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, const DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, int maxWidth, const DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, unsigned int startIndex, unsigned int count, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	DirectX::XMFLOAT2 DrawString(const CharacterSet* characterSet, const std::string& text, DirectX::XMFLOAT2 position, const DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	DirectX::XMFLOAT2 DrawString(const CharacterSet* characterSet, const std::string& text, DirectX::XMFLOAT2 position, unsigned int startIndex, unsigned int count, const DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	void Draw(const Texture2D& texture2D, const Rect& drawRect, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void Draw(const Texture2D& texture2D, const DirectX::XMFLOAT2& position, const Rect& clipRect, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void Draw(const Texture2D& texture2D, const Rect& position, const Rect& clipRect, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void Draw(const Rect& drawRect, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void Draw();

	void Reload();

	void EnableScissorTest(Rect region);
	void DisableScissorTest();

	void SetResolution(int xRes, int yRes);

private:
	ContentManager* contentManager;

	bool hasBegun;

	D3D11_RECT defaultScissorRect;

	//////////////////////////////////////////////////////////////////////////
	//BUFFER VARIABLES
	//////////////////////////////////////////////////////////////////////////
	ID3D11DeviceContext* context;

	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;
	ID3D11SamplerState* samplerState;

	ID3D11RasterizerState* rasterizerState;
	ID3D11BlendState* blendState;
	ID3D11DepthStencilState* depthStencilState;
	
	//Defaults to restore
	ID3D11RasterizerState* defaultRasterizerState;
	ID3D11BlendState* defaultBlendState;
	float defaultBlendFactors[4];
	UINT defaultBlendMask;
	ID3D11DepthStencilState* defaultDepthStencilState;
	UINT defaultStencilRef;

	//SimpleShaderProgram shaderProgram;
	VertexShader vertexShader;
	PixelShader pixelShader;


	COMUniquePtr<ID3D11Buffer> viewProjBuffer;
	DirectX::XMFLOAT4X4 projectionMatrix;

	const int MAX_BUFFER_INSERTS = 2048;

	//Vertex2D = 32 bytes (with padding)
	const unsigned int MAX_VERTEX_BUFFER_INSERTS = MAX_BUFFER_INSERTS * 4;
	const unsigned int VERTEX_BUFFER_SIZE = MAX_VERTEX_BUFFER_INSERTS * sizeof(Vertex2D);
	const unsigned int MAX_INDEX_BUFFER_INSERTS = MAX_BUFFER_INSERTS * 6;
	const unsigned int INDEX_BUFFER_SIZE = MAX_INDEX_BUFFER_INSERTS * sizeof(unsigned int);

	unsigned int vertexBufferArrayInserts;
	unsigned int bufferInserts;

	std::vector<SpriteBatch> spriteBatch;

	unsigned int vertexDataStreamOffset;
	unsigned int indexDataStreamOffset;
	unsigned int drawOffset;

	//For easy drawing of rectangles via Draw()
	Texture2D* whiteTexture;
	Rect whiteTextureClipRect;

	Rect resolutionRect;

	ID3D11Resource* currentTexture;
};

#endif // SpriteRenderer_h__

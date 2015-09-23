#ifndef Texture2D_h__
#define Texture2D_h__

#include "content.h"

#include <DirectXMath.h>

class Texture2D : public Content
{
public:
	Texture2D();
	~Texture2D() = default;

	ID3D11Texture2D* GetTexture() const;
	ID3D11ShaderResourceView* GetTextureResourceView() const;

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

	DirectX::XMINT2 GetSize() const;
	DirectX::XMFLOAT2 GetPredivSize() const;

	friend bool operator==(const Texture2D& lhs, const Texture2D& rhs);
	friend bool operator!=(const Texture2D& lhs, const Texture2D& rhs);
private:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* textureResourceView;

	DirectX::XMINT2 size;
	DirectX::XMFLOAT2 predivSize;

	bool Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager = nullptr, ContentParameters* contentParameters = nullptr) override;
	void Unload(ContentManager* contentManager = nullptr) override;
};

inline bool operator==(const Texture2D& lhs, const Texture2D& rhs)
{
	return lhs.texture == rhs.texture;
}

inline bool operator!=(const Texture2D& lhs, const Texture2D& rhs)
{
	return !(lhs == rhs);
}

#endif // Texture2D_h__

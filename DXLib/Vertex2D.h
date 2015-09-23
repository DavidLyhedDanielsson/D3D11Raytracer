#ifndef Vertex2D_h__
#define Vertex2D_h__

#include <cstdint>

#include <DirectXMath.h>

class Vertex2D
{
public:
	Vertex2D()
		: position(0.0f, 0.0f)
		, texCoords(0.0f, 0.0)
		, color(0.0f, 0.0f, 0.0f, 0.0f)
	{}
	Vertex2D(DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 texCoords, DirectX::XMFLOAT4 color)
		: position(position)
		, texCoords(texCoords)
		, color(color)
	{}
	Vertex2D(float posX, float posY, float texCoordsX, float texCoordsY, DirectX::XMFLOAT4 color)
		: position(posX, posY)
		, texCoords(texCoordsX, texCoordsY)
		, color(color)
	{}
	~Vertex2D() = default;

	DirectX::XMFLOAT2 position; //8 bytes
	DirectX::XMFLOAT2 texCoords; //8 bytes
	DirectX::XMFLOAT4 color; //16 bytes
};

#endif // Vertex2D_h__
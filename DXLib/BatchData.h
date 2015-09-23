#ifndef BatchData_h__
#define BatchData_h__

#include <DirectXMath.h>

class BatchData
{
public:
	BatchData()
		:positionMin(0, 0)
		, positionMax(0, 0)
		, texCoordsMin(0, 0)
		, texCoordsMax(0, 0)
		, color(0, 0, 0, 0)
	{};
	BatchData(const DirectX::XMFLOAT2& positionMin
		, const DirectX::XMFLOAT2& positionMax
		, const DirectX::XMFLOAT2& texCoordsMin
		, const DirectX::XMFLOAT2& texCoordsMax
		, const DirectX::XMFLOAT4& color)
		:positionMin(positionMin)
		, positionMax(positionMax)
		, texCoordsMin(texCoordsMin)
		, texCoordsMax(texCoordsMax)
		, color(color)
	{};
	~BatchData() {};

	DirectX::XMFLOAT2 positionMin;
	DirectX::XMFLOAT2 positionMax;

	DirectX::XMFLOAT2 texCoordsMin;
	DirectX::XMFLOAT2 texCoordsMax;

	DirectX::XMFLOAT4 color;
};

#endif // BatchData_h__

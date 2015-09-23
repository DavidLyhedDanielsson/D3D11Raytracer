#ifndef Rect_h__
#define Rect_h__

#include <DirectXMath.h>

class Rect
{
public:
	Rect();
	Rect(float x, float y, float width, float height);
	Rect(float x, float y, const DirectX::XMFLOAT2& size);
	Rect(const DirectX::XMFLOAT2& position, float width, float height);
	Rect(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size);
	~Rect() = default;

	void Set(float x, float y, float width, float height);
	void Set(float x, float y, const DirectX::XMFLOAT2& size);
	void Set(const DirectX::XMFLOAT2& position, float width, float height);
	void Set(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size);

	void SetPos(float x, float y);
	void SetPos(const DirectX::XMFLOAT2& position);

	void SetSize(float width, float height);
	void SetSize(const DirectX::XMFLOAT2& size);

	bool Contains(float x, float y) const;
	bool Contains(const DirectX::XMFLOAT2& position) const;

	DirectX::XMFLOAT2 GetMinPosition() const;
	DirectX::XMFLOAT2 GetMaxPosition() const;
	DirectX::XMFLOAT2 GetSize() const;

	float GetWidth() const;
	float GetHeight() const;

	Rect& operator+=(const DirectX::XMFLOAT2& rhs);
	Rect& operator-=(const DirectX::XMFLOAT2& rhs);
	Rect& operator*=(const DirectX::XMFLOAT2& rhs);
	Rect& operator/=(const DirectX::XMFLOAT2& rhs);

	bool AlmostEqual(const Rect& rhs, float epsilon) const;

	friend bool operator==(const Rect& lhs, const Rect& rhs);
	friend bool operator!=(const Rect& lhs, const Rect& rhs);

	const static Rect empty;

private:
	DirectX::XMFLOAT2 positionMin;
	DirectX::XMFLOAT2 positionMax;
	DirectX::XMFLOAT2 size;

	void UpdatePositionMax();
};

inline Rect operator+(Rect lhs, const DirectX::XMFLOAT2& rhs)
{
	lhs += rhs;
	return lhs;
}

inline Rect operator-(Rect lhs, const DirectX::XMFLOAT2& rhs)
{
	lhs -= rhs;
	return lhs;
}

inline Rect operator*(Rect lhs, const DirectX::XMFLOAT2& rhs)
{
	lhs *= rhs;
	return lhs;
}

inline Rect operator/(Rect lhs, const DirectX::XMFLOAT2& rhs)
{
	lhs /= rhs;
	return lhs;
}

inline bool operator==(const Rect& lhs, const Rect& rhs)
{
	DirectX::XMVECTOR xmLhsMin = DirectX::XMLoadFloat2(&lhs.positionMin);
	DirectX::XMVECTOR xmLhsMax = DirectX::XMLoadFloat2(&lhs.positionMin);

	DirectX::XMVECTOR xmRhsMin = DirectX::XMLoadFloat2(&rhs.positionMin);
	DirectX::XMVECTOR xmRhsMax = DirectX::XMLoadFloat2(&rhs.positionMin);

	return DirectX::XMVector2Equal(xmLhsMin, xmRhsMin) && DirectX::XMVector2Equal(xmLhsMax, xmRhsMax);
}

inline bool operator!=(const Rect& lhs, const Rect& rhs)
{
	return !(lhs == rhs);
}

#endif // Rect_h__

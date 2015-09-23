#include "rect.h"

Rect::Rect()
	: positionMin(0.0f, 0.0f)
	, positionMax(0.0f, 0.0f)
	, size(0.0f, 0.0f)
{
}

Rect::Rect(float x, float y, float width, float height)
	: positionMin(x, y)
	, positionMax(0.0f, 0.0f)
	, size(width, height)
{
	UpdatePositionMax();
}

Rect::Rect(float x, float y, const DirectX::XMFLOAT2& size)
	: positionMin(x, y)
	, positionMax(0.0f, 0.0f)
	, size(size)
{
	UpdatePositionMax();
}

Rect::Rect(const DirectX::XMFLOAT2& position, float width, float height)
	: positionMin(position)
	, positionMax(0.0f, 0.0f)
	, size(width, height)
{
	UpdatePositionMax();
}

Rect::Rect(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size)
	: positionMin(position)
	, positionMax(0.0f, 0.0f)
	, size(size)
{
	UpdatePositionMax();
}

void Rect::Set(float x, float y, float width, float height)
{
	SetPos(x, y);
	SetSize(width, height);
}

void Rect::Set(float x, float y, const DirectX::XMFLOAT2& size)
{
	SetPos(x, y);
	SetSize(size);
}

void Rect::Set(const DirectX::XMFLOAT2& position, float width, float height)
{
	SetPos(position);
	SetSize(width, height);
}

void Rect::Set(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size)
{
	SetPos(position);
	SetSize(size);
}

void Rect::SetPos(float x, float y)
{
	positionMin.x = x;
	positionMin.y = y;

	UpdatePositionMax();
}

void Rect::SetPos(const DirectX::XMFLOAT2& position)
{
	positionMin = position;

	UpdatePositionMax();
}

void Rect::SetSize(float width, float height)
{
	size.x = width;
	size.y = height;

	UpdatePositionMax();
}

void Rect::SetSize(const DirectX::XMFLOAT2& size)
{
	this->size = size;

	UpdatePositionMax();
}

bool Rect::Contains(float x, float y) const
{
	//Intentionally left as x < positionMax.x to make it zero-indexed
	return (x >= positionMin.x
		&& x < positionMax.x
		&& y >= positionMin.y
		&& y < positionMax.y);
}

bool Rect::Contains(const DirectX::XMFLOAT2& position) const
{
	//Intentionally left as x < positionMax.x to make it zero-indexed
	return (position.x >= positionMin.x
		&& position.x < positionMax.x
		&& position.y >= positionMin.y
		&& position.y < positionMax.y);
}

DirectX::XMFLOAT2 Rect::GetMinPosition() const
{
	return positionMin;
}

DirectX::XMFLOAT2 Rect::GetMaxPosition() const
{
	return positionMax;
}

DirectX::XMFLOAT2 Rect::GetSize() const
{
	return size;
}

float Rect::GetWidth() const
{
	return size.x;
}

float Rect::GetHeight() const
{
	return size.y;
}

Rect& Rect::operator+=(const DirectX::XMFLOAT2& rhs)
{
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmAddPosition = DirectX::XMLoadFloat2(&rhs);

	DirectX::XMStoreFloat2(&this->positionMin, DirectX::XMVectorAdd(xmPosition, xmAddPosition));

	UpdatePositionMax();

	return *this;
}

Rect& Rect::operator-=(const DirectX::XMFLOAT2& rhs)
{
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmAddPosition = DirectX::XMLoadFloat2(&rhs);

	DirectX::XMStoreFloat2(&this->positionMin, DirectX::XMVectorSubtract(xmPosition, xmAddPosition));

	UpdatePositionMax();

	return *this;
}

Rect& Rect::operator*=(const DirectX::XMFLOAT2& rhs)
{
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmAddPosition = DirectX::XMLoadFloat2(&rhs);

	DirectX::XMStoreFloat2(&this->positionMin, DirectX::XMVectorMultiply(xmPosition, xmAddPosition));

	UpdatePositionMax();

	return *this;
}

Rect& Rect::operator/=(const DirectX::XMFLOAT2& rhs)
{
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmAddPosition = DirectX::XMLoadFloat2(&rhs);

	DirectX::XMStoreFloat2(&this->positionMin, DirectX::XMVectorDivide(xmPosition, xmAddPosition));

	UpdatePositionMax();

	return *this;
}

bool Rect::AlmostEqual(const Rect& rhs, float epsilon) const
{
	DirectX::XMVECTOR xmThisMin = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmThisMax = DirectX::XMLoadFloat2(&this->positionMax);

	DirectX::XMVECTOR xmRhsMin = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmRhsMax = DirectX::XMLoadFloat2(&this->positionMax);

	DirectX::XMVECTOR xmEpsilon = DirectX::XMVectorReplicate(epsilon);

	return DirectX::XMVector2NearEqual(xmThisMin, xmRhsMin, xmEpsilon) && DirectX::XMVector2NearEqual(xmThisMin, xmRhsMin, xmEpsilon);
}

const Rect Rect::empty(0.0f, 0.0f, 0.0f, 0.0f);

void Rect::UpdatePositionMax()
{
	DirectX::XMVECTOR xmPositionMin = DirectX::XMLoadFloat2(&this->positionMin);
	DirectX::XMVECTOR xmSize = DirectX::XMLoadFloat2(&this->size);

	DirectX::XMStoreFloat2(&positionMax, DirectX::XMVectorAdd(xmPositionMin, xmSize));
}

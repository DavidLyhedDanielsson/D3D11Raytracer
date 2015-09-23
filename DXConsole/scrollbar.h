#ifndef Scrollbar_h__
#define Scrollbar_h__

#include "guiContainer.h"

#include "scrollbarStyle.h"
#include "guiManager.h"

class Scrollbar :
	public GUIContainer
{
friend class GUIManager;
public:
	Scrollbar();
	~Scrollbar() = default;

	void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle) override;
	void Init(const Rect& area
						, const std::shared_ptr<GUIStyle>& style
						, std::unique_ptr<GUIBackground>& background
						, const std::shared_ptr<GUIStyle>& backgroundStyle
						, std::function<void()> scrollCallback);
	void Update(std::chrono::nanoseconds delta) override;
	void DrawBackground(SpriteRenderer* spriteRenderer) override;
	void DrawMiddle(SpriteRenderer* spriteRenderer) override;

	void Scroll(int index);
	void ScrollTo(int index);

	//void SetStyle(ScrollbarStyle style);
	void SetVisibleItems(int visibleItems);
	void SetMaxItems(int maxItems);

	int GetVisibleItems() const;
	int GetMaxItems() const;

	int GetMinIndex() const;
	int GetMaxIndex() const;

	void SetPosition(const DirectX::XMFLOAT2& newPosition) override;
	void SetPosition(float x, float y) override;
	void SetSize(const DirectX::XMFLOAT2& newSize) override;
	void SetSize(float x, float y) override;
	void SetArea(const Rect& newArea) override;

	void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	void OnScroll(int distance) override;

	bool Contains(const DirectX::XMFLOAT2& position) const;

	static GUIStyle* GenerateDefaultStyle(ContentManager* contentManager);
	static GUIBackground* GenerateDefaultBackground(ContentManager* contentManager);
	static GUIStyle* GenerateDefaultBackgroundStyle(ContentManager* contentManager);
protected:
	std::function<void()> scrollCallback;

	bool mouseDown;
	bool lockedToBottom;

	int beginIndex;
	int endIndex;

	int visibleItems;
	int maxItems;

	//////////////////////////////////////////////////////////////////////////
	//THUMB
	//////////////////////////////////////////////////////////////////////////
	Rect thumb;

	int grabY;

	std::shared_ptr<ScrollbarStyle> style;

	//************************************
	// Method:      UpdateThumbSize
	// FullName:    Scrollbar::UpdateThumbSize
	// Access:      protected 
	// Returns:     void
	// Qualifier:  
	// Description: Updates the thumb size based on visible items and max items
	//************************************
	void UpdateThumbSize();
	//************************************
	// Method:      UpdateThumbIndex
	// FullName:    Scrollbar::UpdateThumbIndex
	// Access:      protected 
	// Returns:     void
	// Qualifier:  
	// Description: Updates the thumb index based on its position
	//************************************
	void UpdateThumbIndex();
	//************************************
	// Method:      UpdateThumbPosition
	// FullName:    Scrollbar::UpdateThumbPosition
	// Access:      protected 
	// Returns:     void
	// Qualifier:  
	// Description: Updates the thumb position based on its index
	//************************************
	void UpdateThumbPosition();

	void ClampThumbPosition();

	int GetEffectiveHeight();
	float GetThumbRelativeY();
};

#endif // Scrollbar_h__

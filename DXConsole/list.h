#ifndef List_h__
#define List_h__

#include "guiContainer.h"
#include "guiManager.h"
#include "scrollbar.h"

#include "listStyle.h"

#include <vector>

class List :
	public GUIContainer
{
friend class GUIManager;
public:
	List();
	virtual ~List();

	//Tell David if you have a good reason to copy
	List& operator=(const List&) = delete;
	List(const List&) = delete;

	virtual void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle) override;
	virtual void Update(std::chrono::nanoseconds delta) override;

	virtual void Draw(SpriteRenderer* spriteRenderer) override;
	virtual void DrawBackground(SpriteRenderer* spriteRenderer) override;
	virtual void DrawMiddle(SpriteRenderer* spriteRenderer) override;
	virtual void DrawForeground(SpriteRenderer* spriteRenderer) override;

	virtual void SetPosition(const DirectX::XMFLOAT2& newPosition) override;
	virtual void SetPosition(float x, float y) override;

	virtual void SetSize(const DirectX::XMFLOAT2& newSize) override;
	virtual void SetSize(float x, float y) override;

	virtual void SetArea(const Rect& newArea) override;

	//void AddElement(GUIContainer* element); //TODO: If this is ever needed make sure to pay attention to shared_ptr
	void SetElements(const std::vector<GUIContainer*> elements);
	std::vector<GUIContainer*> GetElements();
	void ClearElements();

	void HighlightElement(int index);
	void UnHighlightElement();
	void ClearHighlights();
	int GetHighlitElementIndex();
	GUIContainer* GetHighlitElement();

	void SetIgnoreMouse(bool ignore);

	virtual void OnMouseEnter() override;
	virtual void OnMouseExit() override;
	virtual void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	virtual void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	virtual void OnKeyDown(const KeyState& keyState) override;
	virtual void OnKeyUp(const KeyState& keyState) override;
	virtual void OnChar(unsigned int keyCode) override;
	virtual void OnScroll(int distance) override;

	int GetElementsSize();
	bool GetIsScrolling();

	static GUIStyle* GenerateDefaultStyle(ContentManager* contentManager);
	static GUIBackground* GenerateDefaultBackground(ContentManager* contentManager);
	static GUIStyle* GenerateDefaultBackgroundStyle(ContentManager* contentManager);
private:
	std::vector<GUIContainer*> elements;

	GUIContainer* focusOn;

	std::shared_ptr<ListStyle> style;

	Scrollbar scrollbar;
	int drawBegin;
	int drawEnd;

	int elementHeight;

	//TODO: Support multiple highlights
	int highlitElement;

	bool ignoreMouse;
	bool scrolling;

	void UpdatePositions();

	void ScrollFunction();
};

#endif // List_h__

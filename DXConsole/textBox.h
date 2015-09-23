#ifndef TextBox_h__
#define TextBox_h__

#include "guiContainer.h"

#include <DXLib/characterSet.h>
#include <DXLib/constructedString.h>
#include <DXLib/timer.h>

#include <DirectXMath.h>

#include "textBoxStyle.h"
#include "guiManager.h"

class TextBox :
	public GUIContainer
{
friend class GUIManager;
public:
	TextBox();
	virtual ~TextBox();

	bool allowEdit;

	virtual void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle) override;
	virtual void Update(std::chrono::nanoseconds delta) override;
	//virtual void Draw(SpriteRenderer* spriteRenderer) override;

	virtual void DrawBackground(SpriteRenderer* spriteRenderer) override;
	virtual void DrawMiddle(SpriteRenderer* spriteRenderer) override;
	virtual void DrawForeground(SpriteRenderer* spriteRenderer) override;

	void Insert(int index, unsigned int character);
	void Insert(unsigned int index, const std::string& text);
	void Insert(unsigned int index, const char* text);
	void Erase(unsigned int startIndex, unsigned int count);

	void Activate() override;
	void Deactivate() override;

	virtual void OnMouseEnter() override;
	virtual void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	virtual void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	virtual void OnKeyDown(const KeyState& keyState) override;
	virtual void OnKeyUp(const KeyState& keyState) override;
	virtual void OnChar(unsigned int keyCode) override;

	virtual void SetText(const std::string& text);
	std::string GetText() const;
	std::string GetSelectedText() const;
	int GetCharacterAt(unsigned int index) const;
	int GetTextLength() const;
	int GetCursorIndex() const;
	void SetCursorIndex(unsigned int newIndex);

	void SetJumpSeparators(const std::string& separators);
	void SetJumpToBeforeSeparator(bool jump);

	void SetCharacterSet(const std::string& fontPath, ContentManager* contentManager);
	virtual void SetCharacterSet(const CharacterSet* characterSet);

	bool GetIsEmpty() const;
	bool GetSelectionMade() const;

	bool GetIsActive() const;

	void ReplaceActiveCharacterBlockText(std::string newText);
	std::string GetActiveCharacterBlockText();
	std::string GetActiveCharacterBlockText(unsigned int& index);

	static GUIStyle* GenerateDefaultStyle(ContentManager* contentManager);
	static GUIBackground* GenerateDefaultBackground(ContentManager* contentManager);
	static GUIStyle* GenerateDefaultBackgroundStyle(ContentManager* contentManager);
protected:
	ConstructedString constructedString;

	std::shared_ptr<TextBoxStyle> style;

	//Used as a general flag. If cursor is drawn then input should be accepted and processed
	bool drawCursor;
	bool mouseDown;

	//////////////////////////////////////////////////////////////////////////
	//CURSOR
	//////////////////////////////////////////////////////////////////////////
	DirectX::XMFLOAT4 cursorColor;

	int cursorIndex;

	int selectionIndex;
	int selectionStartIndex;
	int selectionEndIndex;
		
	Timer cursorBlinkTimer;
	const std::chrono::nanoseconds cursorBlinkTime = std::chrono::milliseconds(750);

	float xOffset;

	//A jump seprator controls where the cursor should jump when you press CTRL + left or CTRL + right
	//If the separator is space it will jump to the beginning or the end of a word etc.
	std::string jumpSeparators;
	bool jumpToBeforeSeparator;

	void LeftPressed(const KeyState& keyState);
	void RightPressed(const KeyState& keyState);
	void BackspacePressed(const KeyState& keyState);
	void DeletePressed(const KeyState& keyState);

	void MoveCursorLeft();
	void MoveCursorRight();
	void JumpCursorLeft();
	void JumpCursorRight();

	void ExtendSelectionToCursor();
	void BeginSelection();
	void Deselect();
	void EraseSelection();
	bool SelectionMade() const;

	void SetXOffset();
};

#endif // TextBox_h__

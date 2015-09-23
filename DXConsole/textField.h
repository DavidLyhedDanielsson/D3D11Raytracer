#ifndef TextField_h__
#define TextField_h__

#include "guiContainer.h"

#include <DXLib/rect.h>
#include <DXLib/Timer.h>
#include "textFieldStyle.h"
#include "scrollbar.h"
#include "guiManager.h"

class TextField :
	public GUIContainer
{
	friend class GUIManager;
public:
	TextField();
	~TextField() = default;

	void Init(Rect area
		, const std::shared_ptr<GUIStyle>& style
		, std::unique_ptr<GUIBackground>& background
		, const std::shared_ptr<GUIStyle>& backgroundStyle);
	void Update(std::chrono::nanoseconds delta) override;

	//Draws the background as well as text
	void DrawBackground(SpriteRenderer* spriteRenderer) override;
	//Draws any selection highlight
	void DrawMiddle(SpriteRenderer* spriteRenderer) override;
	//Draws cursor
	void DrawForeground(SpriteRenderer* spriteRenderer) override;

	void AddText(std::string text);
	void AppendText(const std::string& text);
	void InsertText(const std::string& text, int lineIndex, int index);

	std::vector<std::string> GetText() const;
	std::vector<std::string> GetLines(int begin, int count) const;
	std::string	GetLine(int stringIndex) const;
	std::string	GetSelectedText() const;
	void EraseLines(int begin, int count);
	int	GetLineCount() const;

	void SetText(const std::string& text);

	void Clear();

	void Activate() override;
	void Deactivate() override;

	bool GetIsActive() const;

	bool allowEdit;

	void SetPosition(const DirectX::XMFLOAT2& newPosition) override;
	void SetPosition(float x, float y) override;
	void SetSize(const DirectX::XMFLOAT2& newSize) override;
	void SetSize(float x, float y) override;

	void SetJumpSeparators(const std::string& separators);
	void SetJumpToBeforeSeparator(bool jump);

	void OnKeyDown(const KeyState& keyState) override;
	void OnMouseEnter() override;
	void OnMouseExit() override;
	void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	void OnChar(unsigned keyCode) override;
	void OnScroll(int distance) override;

	static GUIStyle* GenerateDefaultStyle(ContentManager* contentManager);
	static GUIBackground* GenerateDefaultBackground(ContentManager* contentManager);
	static GUIStyle* GenerateDefaultBackgroundStyle(ContentManager* contentManager);

protected:
	//<text, whether or not the line at this index and the line at the next index is "together">
	//I also realized now that I could've as well used std::pair. TODO
	std::vector<std::tuple<std::string, bool>> lines;

	DirectX::XMFLOAT4 textColor;

	std::shared_ptr<TextFieldStyle> style;

	bool drawCursor;
	bool mouseDown;
	//If focus == true then process input
	bool focus;

	Scrollbar scrollbar;
		
	int yOffset;

	//////////////////////////////////////////////////////////////////////////
	//CURSOR
	//////////////////////////////////////////////////////////////////////////
	DirectX::XMFLOAT4 cursorColor;

	int cursorIndex;
	int cursorLineIndex;

	int selectionIndex;
	int selectionStartIndex;
	int selectionEndIndex;

	int selectionLineIndex;
	int selectionStartLineIndex;
	int selectionEndLineIndex;

	int visibleStringsBegin;
	int visibleStringsEnd;
	int visibleStringsMax;

	int matchWidth;

	Timer cursorBlinkTimer;
	const std::chrono::nanoseconds cursorBlinkTime = std::chrono::milliseconds(750); //Nanoseconds

	DirectX::XMFLOAT2 cursorPosition;

	//A jump seprator controls where the cursor should jump when you press CTRL + left or CTRL + right
	//If the separator is space it will jump to the beginning or the end of a word
	std::string jumpSeparators;
	bool jumpToBeforeSeparator;

	int GetCursorIndex(int width, int line) const;

	void UpPressed(const KeyState& keyState);
	void DownPressed(const KeyState& keyState);
	void LeftPressed(const KeyState& keyState);
	void RightPressed(const KeyState& keyState);
	void BackspacePressed(const KeyState& keyState);
	void DeletePressed(const KeyState& keyState);
	void HomePressed(const KeyState& keyState);
	void EndPressed(const KeyState& keyState);

	void SetCursorIndex(int newIndex);
	void SetCursorLineIndex(int newIndex);
	void MoveCursorUp();
	void MoveCursorDown();
	void MoveCursorLeft();
	void MoveCursorRight();
	void JumpCursorLeft();
	void JumpCursorRight();

	void ExtendSelectionToCursor();
	void BeginSelection();
	void Deselect();
	void EraseSelection();
	bool SelectionMade() const;

	void UpdateText(int beginAtLine);
	void UpdateVisibleStrings();
		
	std::vector<std::tuple<std::string, bool>> CreateLineData(const std::string& text) const;

	void ScrollbarScrolled();
};

#endif // TextField_h__
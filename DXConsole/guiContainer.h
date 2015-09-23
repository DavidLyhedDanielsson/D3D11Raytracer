#ifndef GUIContainer_h__
#define GUIContainer_h__

#include <chrono>

#include <DXLib/rect.h>
#include <DXLib/spriteRenderer.h>
#include <DXLib/keyState.h>

#include "guiStyle.h"
#include "guiBackground.h"

#include <DXLib/logger.h>

class GUIContainer
{
	friend class GUIManager;
public:
	GUIContainer();
	virtual ~GUIContainer();

	//************************************
	// Method:		Init
	// FullName:	GUIContainer::Init
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	float x - position
	// Argument:	float y - position
	// Argument:	float width
	// Argument:	float height
	// Description:	Sets the area of this object
	//************************************
	virtual void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle);

	virtual void Update(std::chrono::nanoseconds delta) {};
	virtual void Draw(SpriteRenderer* spriteRenderer);

	virtual void DrawBackground(SpriteRenderer* spriteRenderer) {}
	virtual void DrawMiddle(SpriteRenderer* spriteRenderer) {}
	virtual void DrawForeground(SpriteRenderer* spriteRenderer) {}

	virtual void SetPosition(const DirectX::XMFLOAT2& newPosition);
	virtual void SetPosition(float x, float y);
	virtual void SetSize(const DirectX::XMFLOAT2& newSize);
	virtual void SetSize(float x, float y);
	//************************************
	// Method:		SetArea
	// FullName:	GUIContainer::SetArea
	// Access:		virtual public 
	// Returns:		void
	// Qualifier:	
	// Argument:	const Rect& newArea
	// Description:	Sets the area of this object. This is SetPosition() and SetSize() combined
	//************************************
	virtual void SetArea(const Rect& newArea);

	//TODO: GetWorkArea() const;

	virtual Rect GetArea() const;
	virtual DirectX::XMFLOAT2 GetPosition() const;
	virtual DirectX::XMFLOAT2 GetSize() const;

	virtual bool GetDraw() const;
	virtual void SetDraw(bool draw);

	virtual bool GetUpdate() const;
	virtual void SetUpdate(bool update);

	virtual bool GetReceiveAllEvents() const;
	virtual void SetReceiveAllEvents(bool active);

	virtual void UnHighlight() {};
	virtual void Highlight() {};

	virtual void Activate() {};
	virtual void Deactivate() {};

	//************************************
	// Method:		OnMouseEnter
	// FullName:	GUIContainer::OnMouseEnter
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Description:	Called whenever the cursor enters "area". Should be protected
	//************************************
	virtual void OnMouseEnter() {}; //TODO: This and OnMouseExit should probably be protected
	//************************************
	// Method:		OnMouseExit
	// FullName:	GUIContainer::OnMouseExit
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Description:	Called whenever the cursor exits "area". Should be protected
	//************************************
	virtual void OnMouseExit() {};
	//************************************
	// Method:		OnMouseDown
	// FullName:	GUIContainer::OnMouseDown
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	KeyState keyState
	// Description:	Called whenever a mouse button is pressed and "recieveAllEvents" is true
	//************************************
	virtual void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) {};
	//************************************
	// Method:		OnMouseUp
	// FullName:	GUIContainer::OnMouseUp
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	KeyState keyState
	// Description:	Called whenever a mouse button is released and "recieveAllEvents" is true
	//************************************
	virtual void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) {};
	//************************************
	// Method:		OnKeyDown
	// FullName:	GUIContainer::OnKeyDown
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	KeyState keyState
	// Description:	Called whenever a button is pressed and "recieveAllEvents" is true
	//************************************
	virtual void OnKeyDown(const KeyState& keyState) {};
	//************************************
	// Method:		OnKeyUp
	// FullName:	GUIContainer::OnKeyUp
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	KeyState keyState
	// Description:	Called whenever a button is released and "recieveAllEvents" is true
	//************************************
	virtual void OnKeyUp(const KeyState& keyState) {};
	//************************************
	// Method:		OnChar
	// FullName:	GUIContainer::OnChar
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	KeyState keyState
	// Description:	Called whenever the user presses a button to enter text and "recieveAllEvents" is true
	//************************************
	virtual void OnChar(unsigned int keyCode) {};
	//************************************
	// Method:		OnScroll
	// FullName:	GUIContainer::OnScroll
	// Access:		virtual protected
	// Returns:		void
	// Qualifier:
	// Argument:	double x
	// Argument:	double y
	// Description:	Called whenever the user scrolls and "recieveAllEvents" is true
	//************************************
	virtual void OnScroll(int distance) {};

protected:
	//If "recieveAllEvents" is true this object will recieve mouse/key events.
	//If not, this will only recieve OnMouseEnter and OnMouseExit
	bool recieveAllEvents;
	//Should this object's Update be called?
	bool update;
	//Should this object's Draw be called?
	bool draw;

	std::unique_ptr<GUIBackground> background;

	//Essentially the hitbox of this object
	Rect area;

	//************************************
	// Method:		CastStyleTo
	// Argument:	GUIStyle* style
	// Returns:		bool - whether the cast was successful or not
	// Description: Tries to cast the given style to the given type, if the cast fails an error message is logged
	//************************************
	template<typename T>
	bool CastStyleTo(GUIStyle* style, LOG_TYPE logType)
	{
		if(dynamic_cast<T*>(style) == nullptr)
		{
			Logger::LogLine(logType, "Couldn't cast style to " + std::string(typeid(T).name()));
			return false;
		}

		return true;
	}
private:
	bool mouseInside;
};

#endif // GUIContainer_h__

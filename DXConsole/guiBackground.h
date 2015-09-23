#ifndef OPENGLWINDOW_BACKGROUND_H
#define OPENGLWINDOW_BACKGROUND_H

#include "guiStyle.h"

#include <memory>

#include <DXLib/spriteRenderer.h>

class GUIBackground
{
public:
	GUIBackground();
	virtual ~GUIBackground() = default;

	virtual void Init(const std::shared_ptr<GUIStyle>& style, const Rect* area) = 0;
	virtual void Draw(SpriteRenderer* spriteRenderer) = 0;

	virtual void AreaChanged();
	virtual void ChangePreset(int preset);

	//virtual Rect SetWorkArea(const Rect& newArea);
	virtual Rect GetWorkArea() const;
	virtual Rect GetFullArea() const;

	virtual GUIBackground* Clone() = 0;
protected:
	const Rect* area;

	int preset;

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
};

#endif //OPENGLWINDOW_BACKGROUND_H

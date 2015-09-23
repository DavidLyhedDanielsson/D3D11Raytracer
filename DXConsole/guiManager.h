#ifndef GUIManager_h__
#define GUIManager_h__

#include <vector>

#include <DXLib/spriteRenderer.h>

#include "guiContainer.h"

class GUIManager
{
public:
	GUIManager();
	~GUIManager();

	void AddContainer(GUIContainer* container);
	void SetContainers(std::vector<GUIContainer*> containers);

	void KeyEvent(const KeyState& keyState);
	void MouseEvent(const KeyState& keyState);
	void CharEvent(unsigned int keyCode);
	void ScrollEvent(int distance);

	void Update(std::chrono::nanoseconds delta);
	void Draw(SpriteRenderer* spriteRenderer); //TODO: Use DrawBackground etc.

	void ClearContainers();

private:
	std::vector<GUIContainer*> containers;
};

#endif // GUIManager_h__

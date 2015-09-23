#include "guiManager.h"

#include <DXLib/input.h>

GUIManager::GUIManager()
{
}

GUIManager::~GUIManager()
{
}

void GUIManager::AddContainer(GUIContainer* container)
{
	containers.push_back(container);
}

void GUIManager::SetContainers(std::vector<GUIContainer*> containers)
{
	this->containers = containers;
}

void GUIManager::Update(std::chrono::nanoseconds delta)
{
	DirectX::XMFLOAT2 mousePosition = Input::GetMousePosition();
	for(GUIContainer* container : containers)
	{
		if(container->area.Contains(mousePosition))
		{
			if(!container->mouseInside)
			{
				container->OnMouseEnter();
				container->mouseInside = true;
			}
		}
		else
		{
			if(container->mouseInside)
			{
				container->OnMouseExit();
				container->mouseInside = false;
			}
		}

		if(container->GetUpdate())
			container->Update(delta);
	}
}

void GUIManager::Draw(SpriteRenderer* spriteRenderer)
{
	for(GUIContainer* container : containers)
		if(container->draw)
			container->Draw(spriteRenderer);
}

void GUIManager::KeyEvent(const KeyState& keyState)
{
	if(keyState.action == KEY_ACTION::DOWN
		|| keyState.action == KEY_ACTION::REPEAT)
	{
		for(GUIContainer* container : containers)
			if(container->recieveAllEvents)
				container->OnKeyDown(keyState);
	}
	else
	{
		for(GUIContainer* container : containers)
			if(container->recieveAllEvents)
				container->OnKeyUp(keyState);
	}
}

void GUIManager::MouseEvent(const KeyState& keyState)
{
	DirectX::XMFLOAT2 mousePosition = Input::GetMousePosition();

	if(keyState.action == KEY_ACTION::DOWN
		|| keyState.action == KEY_ACTION::REPEAT)
	{
		for(GUIContainer* container : containers)
			if(container->recieveAllEvents)
				container->OnMouseDown(keyState, mousePosition);
	}
	else
	{
		for(GUIContainer*& container : containers)
			if(container->recieveAllEvents)
				container->OnMouseUp(keyState, mousePosition);
	}
}

void GUIManager::CharEvent(unsigned int keyCode)
{
	for(GUIContainer* container : containers)
		if(container->recieveAllEvents)
			container->OnChar(keyCode);
}

void GUIManager::ScrollEvent(int distance)
{
	for(GUIContainer* container : containers)
		if(container->recieveAllEvents)
			container->OnScroll(distance);
}

void GUIManager::ClearContainers()
{
	containers.clear();
}

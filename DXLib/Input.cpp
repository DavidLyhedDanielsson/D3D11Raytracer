
#include "input.h"

#include "logger.h"

HWND Input::listenWindow;

std::function<void(KeyState)> Input::keyCallback;
std::function<void(KeyState)> Input::mouseButtonCallback;
std::function<void(int)> Input::charCallback;
std::function<void(UINT)> Input::scrollCallback;

DirectX::XMFLOAT2 Input::mouseDownPos[3];
DirectX::XMFLOAT2 Input::mouseUpPos[3];

DirectX::XMFLOAT2 Input::mousePosition;
DirectX::XMFLOAT2 Input::oldMousePosition;

UINT Input::lastMouseButton;
Timer Input::lastClickTimer;
std::chrono::milliseconds Input::doubleClickDelay;

Input::Input()
{
	
}

Input::~Input()
{

}

void Input::Init(HWND window)
{
	listenWindow = window;

	for(int i = 0; i < 3; ++i)
	{
		mouseDownPos[i] = DirectX::XMFLOAT2(0.0f, 0.0f);
		mouseUpPos[i] = DirectX::XMFLOAT2(0.0f, 0.0f);
	}

	mousePosition.x = 0;
	mousePosition.y = 0;
	oldMousePosition.x = 0;
	oldMousePosition.y = 0;

	keyCallback = nullptr;
	mouseButtonCallback = nullptr;

	SetDoubleClickDelay(500);

	lastClickTimer.Start();
}

void Input::Update()
{
	oldMousePosition = mousePosition;

	POINT mousePoint;

	GetCursorPos(&mousePoint);
	ScreenToClient(listenWindow, &mousePoint);

	mousePosition.x = static_cast<float>(mousePoint.x);
	mousePosition.y = static_cast<float>(mousePoint.y);
}

//////////////////////////////////////////////////////////////////////////
//CALLBACK REGISTER
//////////////////////////////////////////////////////////////////////////
void Input::RegisterKeyCallback(std::function<void(const KeyState&)> callback)
{
	keyCallback = callback;
}

void Input::RegisterCharCallback(std::function<void(int)> callback)
{
	charCallback = callback;
}

void Input::RegisterScrollCallback(std::function<void(UINT)> callback)
{
	scrollCallback = callback;
}

void Input::RegisterMouseButtonCallback(std::function<void(const KeyState&)> callback)
{
	mouseButtonCallback = callback;
}

void Input::RegisterMouseMoveCallback(std::function<void(int xPos, int yPos)> callback)
{
	//TODO: Use this? Or not?
}

void Input::UnregisterKeyCallback()
{
	keyCallback = nullptr;
}

void Input::UnregisterMouseButtonCallback()
{
	mouseButtonCallback = nullptr;
}

void Input::HideCursor()
{
	//glfwSetInputMode(listenWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::ShowCursor()
{
	//glfwSetInputMode(listenWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

//////////////////////////////////////////////////////////////////////////
//SETTERS
//////////////////////////////////////////////////////////////////////////
void Input::SetDoubleClickDelay(unsigned int ms)
{
	doubleClickDelay = std::chrono::milliseconds(ms);
}

//////////////////////////////////////////////////////////////////////////
//GETTERS
//////////////////////////////////////////////////////////////////////////
DirectX::XMFLOAT2 Input::GetMouseDelta()
{
	DirectX::XMFLOAT2 returnPosition;

	returnPosition.x = mousePosition.x - oldMousePosition.x;
	returnPosition.y = mousePosition.y - oldMousePosition.y;

	return returnPosition;
}

DirectX::XMFLOAT2 Input::GetMouseDelta(DirectX::XMFLOAT2 center)
{
	DirectX::XMFLOAT2 returnPosition;

	returnPosition.x = mousePosition.x - center.x;
	returnPosition.y = mousePosition.y - center.y;

	return returnPosition;
}

DirectX::XMFLOAT2 Input::GetMousePosition()
{
	return mousePosition;
}

bool Input::MouseMoved()
{
	return mousePosition.x != oldMousePosition.x && mousePosition.y != oldMousePosition.y;
}

DirectX::XMFLOAT2 Input::GetMouseDownPos(int button)
{
	return mouseDownPos[button];
}

DirectX::XMFLOAT2 Input::GetMouseUpPos(int button)
{
	return mouseUpPos[button];
}

HWND Input::GetListenWindow()
{
	return listenWindow;
}

void Input::KeyEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(keyCallback == nullptr)
		return;

	KeyState keyState;

	if(msg == WM_KEYDOWN)
	{
		if(((lParam >> 30) & 1) == 0)
			keyState.action = KEY_ACTION::DOWN;
		else
			keyState.action = KEY_ACTION::REPEAT;
	}
	else
		keyState.action = KEY_ACTION::UP;

	keyState.key = static_cast<int>(wParam);

	keyCallback(keyState);
}

void Input::CharEvent(unsigned int key)
{
	if(key < 32)
		return;

	if(charCallback != nullptr)
		charCallback(key);
}

void Input::MouseButtonEvent(UINT msg, WPARAM wParam)
{
	if(mouseButtonCallback == nullptr)
		return;

	KeyState keyState;

	if((wParam & MK_CONTROL) == MK_CONTROL)
		keyState.mods |= KEY_MODIFIERS::CONTROL;
/*
	if((wParam & MK_LBUTTON) == MK_LBUTTON)
		keyState.mods |= KEY_MODIFIERS::L_MOUSE;
	if((wParam & MK_MBUTTON) == MK_MBUTTON)
		keyState.mods |= KEY_MODIFIERS::M_MOUSE;
	if((wParam & MK_RBUTTON) == MK_RBUTTON)
		keyState.mods |= KEY_MODIFIERS::R_MOUSE;*/
	if((wParam & MK_SHIFT) == MK_SHIFT)
		keyState.mods |= KEY_MODIFIERS::SHIFT;
	if((wParam & MK_XBUTTON1) == MK_XBUTTON1)
		keyState.mods |= KEY_MODIFIERS::M_XBUTTON1;
	if((wParam & MK_XBUTTON2 )== MK_XBUTTON2)
		keyState.mods |= KEY_MODIFIERS::M_XBUTTON2;

	switch(msg)
	{
		case WM_LBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::LEFT);
			break;
		case WM_LBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::LEFT);
			break;
		case WM_MBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::MIDDLE);
			break;
		case WM_MBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::MIDDLE);
			break;
		case WM_RBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::RIGHT);
			break;
		case WM_RBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::RIGHT);
			break;
		default:
			break;
	}

	DirectX::XMFLOAT2 mousePoint = GetMousePosition();

	if(keyState.action == KEY_ACTION::DOWN)
	{
		DirectX::XMFLOAT2 newMousePos(mousePoint.x, mousePoint.y);
		DirectX::XMFLOAT2 lastMouseDownPos(mouseDownPos[lastMouseButton].x, mouseDownPos[lastMouseButton].y);

		DirectX::XMVECTOR xmNewMousePos = DirectX::XMLoadFloat2(&newMousePos);
		DirectX::XMVECTOR xmLastMouseDownPos = DirectX::XMLoadFloat2(&lastMouseDownPos);

		if(lastMouseButton == msg
			&& lastClickTimer.GetTime() <= doubleClickDelay
			&& DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSubtract(xmNewMousePos, xmLastMouseDownPos))) <= maxDoubleClickDistance)
			keyState.action = KEY_ACTION::REPEAT;

		lastClickTimer.Reset();

		mouseDownPos[keyState.key] = mousePoint;
	}
	else
	{
		mouseUpPos[keyState.key] = mousePoint;
	}

	lastMouseButton = msg;

	mouseButtonCallback(keyState);
}

void Input::ScrollEvent(UINT distance)
{
	if(scrollCallback != nullptr)
		scrollCallback(distance);
}

DirectX::XMFLOAT2 Input::GetWindowSize()
{
	RECT rect;

	GetClientRect(listenWindow, &rect);

	return DirectX::XMFLOAT2(static_cast<float>(rect.right), static_cast<float>(rect.bottom));
}

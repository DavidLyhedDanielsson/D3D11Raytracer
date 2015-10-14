
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

MOUSE_BUTTON Input::lastMouseButton;
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

	keyState.mods = GetCurrentModifierKeys();

	keyState.key = static_cast<int>(wParam);

	keyCallback(keyState);
}

void Input::CharEvent(unsigned int key)
{
	if(key < 32 || key > 126)
		return;

	if(charCallback != nullptr)
		charCallback(key);
}

void Input::MouseButtonEvent(UINT msg, WPARAM wParam)
{
	if(mouseButtonCallback == nullptr)
		return;

	KeyState keyState;

	keyState.mods = GetCurrentModifierKeys();

	MOUSE_BUTTON currentMouseButton = MOUSE_BUTTON::UNKNOWN;

	switch(msg)
	{
		case WM_LBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::LEFT);
			currentMouseButton = MOUSE_BUTTON::LEFT;
			break;
		case WM_LBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::LEFT);
			currentMouseButton = MOUSE_BUTTON::LEFT;
			break;
		case WM_MBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::MIDDLE);
			currentMouseButton = MOUSE_BUTTON::MIDDLE;
			break;
		case WM_MBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::MIDDLE);
			currentMouseButton = MOUSE_BUTTON::MIDDLE;
			break;
		case WM_RBUTTONDOWN:
			keyState.action = KEY_ACTION::DOWN;
			keyState.key = static_cast<int>(MOUSE_BUTTON::RIGHT);
			currentMouseButton = MOUSE_BUTTON::RIGHT;
			break;
		case WM_RBUTTONUP:
			keyState.action = KEY_ACTION::UP;
			keyState.key = static_cast<int>(MOUSE_BUTTON::RIGHT);
			currentMouseButton = MOUSE_BUTTON::RIGHT;
			break;
		default:
			break;
	}

	DirectX::XMFLOAT2 mousePoint = GetMousePosition();

	if(keyState.action == KEY_ACTION::DOWN)
	{
		DirectX::XMFLOAT2 newMousePos(mousePoint.x, mousePoint.y);
		DirectX::XMFLOAT2 lastMouseDownPos(mouseDownPos[static_cast<int>(lastMouseButton)].x, mouseDownPos[static_cast<int>(lastMouseButton)].y);

		DirectX::XMVECTOR xmNewMousePos = DirectX::XMLoadFloat2(&newMousePos);
		DirectX::XMVECTOR xmLastMouseDownPos = DirectX::XMLoadFloat2(&lastMouseDownPos);

		if(currentMouseButton == lastMouseButton
			&& lastClickTimer.GetTime() <= doubleClickDelay
			&& DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSubtract(xmNewMousePos, xmLastMouseDownPos))) <= maxDoubleClickDistance)
		{
			keyState.action = KEY_ACTION::REPEAT;
		}

		lastClickTimer.Reset();

		mouseDownPos[keyState.key] = mousePoint;
	}
	else
	{
		mouseUpPos[keyState.key] = mousePoint;
	}

	lastMouseButton = currentMouseButton;
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

KEY_MODIFIERS Input::GetCurrentModifierKeys()
{
	int modifiers = 0;

	if(GetKeyState(VK_LSHIFT) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::L_SHIFT);
	if(GetKeyState(VK_RSHIFT) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::R_SHIFT);
	if(GetKeyState(VK_LCONTROL) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::L_CONTROL);
	if(GetKeyState(VK_RCONTROL) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::R_CONTROL);
	if(GetKeyState(VK_LMENU) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::L_ALT);
	if(GetKeyState(VK_RMENU) & 0x8000)
		modifiers |= static_cast<int>(KEY_MODIFIERS::R_ALT);

	return static_cast<KEY_MODIFIERS>(modifiers);
}
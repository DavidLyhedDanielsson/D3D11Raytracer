#ifndef KeyState_h__
#define KeyState_h__

//Do not change before finding usages in Input
enum class KEY_ACTION : int
{
	UP = 0x1
	, DOWN = 0x2
	, REPEAT = 0x4
	, UNKNOWN = 0x0
};

enum class MOUSE_BUTTON : int
{
	LEFT = 0x1
	, RIGHT = 0x2
	, MIDDLE = 0x4
	, UNKNOWN = 0x0
};

enum class KEY_MODIFIERS : int
{
	CONTROL = 0x1
	, L_MOUSE = 0x2
	, M_MOUSE = 0x4
	, R_MOUSE = 0x8
	, SHIFT = 0x10
	, M_XBUTTON1 = 0x20
	, M_XBUTTON2 = 0x40
	, UNKNOWN = 0x0
};

class KeyState
{
public:
	KeyState()
		: key(0)
		, action(KEY_ACTION::UNKNOWN)
		, mods(KEY_MODIFIERS::UNKNOWN)
	{}
	KeyState(int key, KEY_ACTION action)
		: key(key)
		, action(action)
		, mods(KEY_MODIFIERS::UNKNOWN)
	{}
	KeyState(MOUSE_BUTTON key, KEY_ACTION action)
		: key(static_cast<int>(key))
		, action(action)
		, mods(KEY_MODIFIERS::UNKNOWN)
	{}
		~KeyState() = default;

	int key;
	KEY_ACTION action;
	KEY_MODIFIERS mods;
};

inline bool operator==(const KeyState& lhs, const KeyState& rhs)
{
	//GLFW_RELEASE = 0
	//Custom "not pressed" = -1
	//Make sure keys and mods match and that they are both pressed (or clicked)
	return lhs.key == rhs.key && lhs.action == rhs.action;
}

inline bool operator!=(const KeyState& lhs, const KeyState& rhs)
{
	return !operator==(lhs, rhs);
}

inline bool operator==(int lhs, MOUSE_BUTTON rhs)
{
	return lhs == static_cast<int>(rhs);
}

inline bool operator!=(int lhs, MOUSE_BUTTON rhs)
{
	return !(lhs == static_cast<int>(rhs));
}

inline KEY_MODIFIERS operator|(KEY_MODIFIERS lhs, KEY_MODIFIERS rhs)
{
	return static_cast<KEY_MODIFIERS>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline KEY_MODIFIERS& operator|=(KEY_MODIFIERS& lhs, KEY_MODIFIERS rhs)
{
	lhs = static_cast<KEY_MODIFIERS>(static_cast<int>(lhs) | static_cast<int>(rhs));

	return lhs;
}

#endif // KeyState_h__
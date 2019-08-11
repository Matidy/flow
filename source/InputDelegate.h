#pragma once
#include <SDL_scancode.h>

struct InputCoreIF;

class InputDelegate
{
public:
	InputDelegate(InputCoreIF& _inputCore);
	~InputDelegate();

	virtual void DefineHeldInput() {}
	virtual void KeyPressedInput(SDL_Scancode const& _key) {}
	virtual void KeyReleasedInput(SDL_Scancode const& _key) {}
	virtual void DefineChordInput() {}

protected:
	InputCoreIF& m_inputCoreIF;
};
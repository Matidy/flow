#pragma once
#include <SDL_scancode.h>

struct InputCoreIF;

class InputDelegate
{
public:
	InputDelegate(InputCoreIF& _inputCore);
	virtual ~InputDelegate();

	virtual void DefineHeldInput(uint32_t _timeStep) {}
	virtual void KeyPressedInput(SDL_Scancode const& _key) {}
	virtual void KeyReleasedInput(SDL_Scancode const& _key) {}
	virtual void DefineChordInput() {}

protected:
	InputCoreIF& m_inputCoreIF;
};
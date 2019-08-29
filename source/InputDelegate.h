#pragma once
#include <SDL_scancode.h>
#include "Data/flVec2.h"

struct InputCoreIF;

class InputDelegate
{
public:
	InputDelegate(InputCoreIF& _inputCore);
	virtual ~InputDelegate();

	virtual void DefineHeldInput(uint32_t _timeStep) {}
	virtual void MouseMovementInput(flVec2<int> _mousePos) {}
	virtual void MouseUpInput(flVec2<int32_t> _mousePos) {}
	virtual void MouseDownInput(flVec2<int32_t> _mousePos) {}
	virtual void KeyPressedInput(SDL_Scancode const& _key) {}
	virtual void KeyReleasedInput(SDL_Scancode const& _key) {}
	virtual void DefineChordInput(uint32_t _timeStep) {}

protected:
	InputCoreIF& m_inputCoreIF;
};
#pragma once
#include <utility>
#include <SDL_scancode.h>

class InputDelegate;
typedef std::pair<SDL_Scancode, SDL_Scancode> KeyChordPair;

struct InputCoreIF
{
	virtual void SetActiveInputDelegate(InputDelegate * _inputDelegate) = 0;
	virtual void SetGlobalGameInputDelegate(InputDelegate * _inputDelegate) = 0;
	virtual bool const& IsPressed(SDL_Scancode const _keyScanCode) = 0;
	virtual bool const TryKeyChord(KeyChordPair _keyChord) = 0;

	virtual bool const& ToggleInputBatching() = 0;
};
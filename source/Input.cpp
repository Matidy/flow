// #define NDEBUG
#include <cassert>
#include <SDL_events.h>
#include "Input.h"

#include "WorldGrid.h"
#if _DEBUG
#include "Debug.h"
#endif

Input::Input()
	: 
	m_activeChord(KeyChordPair(SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN)),
	m_activeChordProcessed(false)
{
}

bool Input::Init
(
	WorldGrid * const _worldGrid
#if _DEBUG 
	, Debug * const _debug
#endif
)
{
	m_worldGrid = _worldGrid;
#if _DEBUG
	m_debug = _debug;
#endif
	if (m_worldGrid != nullptr
#if _DEBUG
		&& m_debug != nullptr
#endif
		)
	{
		return true;
	}

	return false;
}

void Input::UpdateKeyboardState
(
	SDL_Event const& _e
)
{
	SDL_Scancode const key = _e.key.keysym.scancode;
	switch (_e.type)
	{
	case SDL_KEYDOWN:
	{
		//handle chord input
		if (!ActiveChordWaiting())
		{
			if (key == SDL_SCANCODE_LCTRL)
			{
				//start a chord
				m_activeChord.first = key;
			}
		}
		else if (ActiveChordWaiting()
				&& key != m_activeChord.first) //check its not an event for the first chord key being held down
		{
			if (m_activeChord.second == SDL_SCANCODE_UNKNOWN)
			{
				//complete our waiting chord with this key press
				m_activeChord.second = key;
			}
		}

		//update keyboard state data
		bool& isPressed = m_pressedKeys[key];
		isPressed = true;
		break;
	}

	case SDL_KEYUP:
	{
		//handle chord input
		if (ActiveChordWaiting())
		{
			//active wait chord
			if (key == m_activeChord.first)
			{
				//end current chord
				m_activeChord.first = SDL_SCANCODE_UNKNOWN;
				m_activeChord.second = SDL_SCANCODE_UNKNOWN;
				m_activeChordProcessed = false;
			}
			else if (key == m_activeChord.second)
			{
				//free chord second for use by another key
				m_activeChord.second = SDL_SCANCODE_UNKNOWN;
				m_activeChordProcessed = false;
			}
		}
		// releasing chord pair key should not clear the chord but should free up the chord start key for
		// use with another chord, including another chord input of the same type

		//update keyboard state data
		bool& isPressed = m_pressedKeys[key];
		isPressed = false;
		break;
	}
	}
}

bool const& Input::IsPressed
(
	SDL_Scancode const _keyScanCode
)
{
	return m_pressedKeys[_keyScanCode];
}

//////////////////////////////////////////////////////////
// @brief - returns whether we have successfully consumed the given keyChord
bool const Input::TryKeyChord
(
	KeyChordPair _keyChord
)
{
	//check chord we're trying is well formed
	assert(_keyChord.first != SDL_SCANCODE_UNKNOWN
		&& _keyChord.second != SDL_SCANCODE_UNKNOWN);

	if (!m_activeChordProcessed)
	{
		if (m_activeChord == _keyChord)
		{
			m_activeChordProcessed = true;

			return true;
		}
	}

	return false; //no waiting chord matches given chord
}

void Input::HandleInput
(
)
{
	if (ActiveChordWaiting())
	{
		//multi-key input handling
		if( TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_D)) )
		{
			m_debug->ToggleDebug();
		}
	}
	else
	{
		//single key input handling
#if _DEBUG
		if (m_debug->DebugEnabled())
		{
			HandleDebugInput();
		}
		else
		{
#endif 
			HandleGameInput();
#if _DEBUG
		}
#endif
	}
}

void Input::HandleGameInput
(
)
{
	if (IsPressed(SDL_SCANCODE_LEFT))
	{
		printf("Left Pressed Game Mode\n");
	}
	if (IsPressed(SDL_SCANCODE_RIGHT))
	{
		printf("Right Pressed Game Mode\n");
	}
}

#if _DEBUG
void Input::HandleDebugInput
(
)
{
	if (IsPressed(SDL_SCANCODE_LEFT))
	{
		//@TODO: Decrement propagation step
		printf("Left Pressed Debug Mode\n");
	}
	if (IsPressed(SDL_SCANCODE_RIGHT))
	{
		//@TODO: Increment propagation step
		printf("Right Pressed Debug Mode\n");
	}
}
#endif
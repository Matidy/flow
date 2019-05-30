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
#if _DEBUG
	, m_currInputBatchWaiting(0u)
	, m_inputBatchEnabled(false)
#endif
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
	SDL_Scancode const& key = _e.key.keysym.scancode;
	switch (_e.type)
	{
	case SDL_KEYDOWN:
	{
		if (!IsPressed(key))
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
			else if (ActiveChordWaiting())
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

			KeyPressedEvent(key);
		}

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

		KeyReleasedEvent(key);

		break;
	}
	}
}

void Input::KeyPressedEvent
(
	SDL_Scancode const& _key
)
{
	// debug mode event input handling
	if (m_debug->DebugEnabled())
	{
		if (_key == SDL_SCANCODE_T)
		{
			m_debug->ToggleTessalationDisplay();
		}
		if (_key == SDL_SCANCODE_I)
		{
			m_inputBatchEnabled = !m_inputBatchEnabled;
			m_inputBatchEnabled ? printf("Input Batch Enabled\n") : printf("Input Batch Disabled\n");
		}
	}
	// main game event input handling
	else
	{
		
	}
}

void Input::KeyReleasedEvent
(
	SDL_Scancode const& _key
)
{
	// debug mode event input handling
	if (m_debug->DebugEnabled())
	{
		if (_key == SDL_SCANCODE_T)
		{
			m_debug->ToggleTessalationDisplay();
		}
	}
	// main game event input handling
	else
	{

	}
}


bool const& Input::IsPressed
(
	SDL_Scancode const _keyScanCode
)
{
	return m_pressedKeys[_keyScanCode]; //luckily when no entry exists a new one is created and the bool init'd to false by default which is
										//what we want for calls to this function that are checking if keys that have never been pressed are
										//pressed
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

bool const Input::InputBatchEnabled
(
) const
{
	return m_inputBatchEnabled;
}

void Input::BatchProcessInputs
(
	SDL_Event const _e
)
{
	if (_e.key.keysym.scancode == SDL_SCANCODE_I)
	{
		//ugly hardcoded hack to make sure Input to disable input batching doesn't get input batched so
		//we can turn input batching off easily
		UpdateKeyboardState(_e);
	}
	else if (m_currInputBatchWaiting != m_inputBatchSize)
	{
		// store this input for later processing
		m_inputBatch[m_currInputBatchWaiting] = _e;
		m_currInputBatchWaiting++;
	}
	else
	{
		// batch process target reached, batch process inputs
		for (Uint8 i = 0; i<m_inputBatchSize; i++)
		{
			UpdateKeyboardState(m_inputBatch[i]);
		}

		m_currInputBatchWaiting = 0;
	}
}
#endif
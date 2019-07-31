// #define NDEBUG
#include <cassert>
#include "InputCore.h"

#include "InputDelegate.h"


void InputCore::SetActiveInputDelegate
(
	InputDelegate * _inputDelegate
)
{
	assert(_inputDelegate);
	m_activeInputDelegate = _inputDelegate;

	//@TODO - Handle object owning function pointed to getting destructed?
}

void InputCore::SetGlobalGameInputDelegate(InputDelegate * _inputDelegate)
{
	assert(_inputDelegate);
	m_globalGameInputDelegate = _inputDelegate;
}

bool const& InputCore::IsPressed
(
	SDL_Scancode const _keyScanCode
)
{
	return m_pressedKeys[_keyScanCode]; //luckily, when no entry exists a new one is created and the bool init'd to false by default which is
										//what we want for calls to this function that are checking if keys that have never been pressed are
										//pressed
}

// return - current state of InputBatching after toggle
bool const& InputCore::ToggleInputBatching()
{
	m_inputBatchEnabled = !m_inputBatchEnabled;

	return m_inputBatchEnabled;
}


/////////////////////////////////////////////////////////////////////////////////////
InputCore::InputCore()
	: 
	m_activeInputDelegate(nullptr),
	m_globalGameInputDelegate(nullptr),
	m_activeChord(KeyChordPair(SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN)),
	m_activeChordProcessed(false)
#ifdef _DEBUG
	, m_currInputBatchWaiting(0u)
	, m_inputBatchEnabled(false)
#endif
{
}

void InputCore::UpdateKeyboardState
(
	SDL_Event const& _e
)
{
	SDL_Scancode const& key = _e.key.keysym.scancode;
	switch (_e.type)
	{
	case SDL_KEYDOWN:
	{
		//@TODO - This logic is kinda janky, like still registering a keyPressedEvent & storing a key as
		//being pressed even if a chord is current active - want to either be considering a chord or
		//single key presses, not both at the same time. For checking held down keys, this is handled
		//at the CheckInput level currently, but not at the KeyPressedEvent level
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

			//fire off key pressed event
			if (m_activeInputDelegate)
			{
				m_activeInputDelegate->KeyPressedInput(key);
			}
			if (m_globalGameInputDelegate)
			{
				m_globalGameInputDelegate->KeyPressedInput(key);
			}
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

		//fire off key released event
		if (m_activeInputDelegate)
		{
			m_activeInputDelegate->KeyReleasedInput(key);
		}
		if (m_globalGameInputDelegate)
		{
			m_globalGameInputDelegate->KeyReleasedInput(key);
		}

		break;
	}
	}
}

void InputCore::CheckHeldKeyboardInput()
{
	if (ActiveChordWaiting())
	{
		//chord input
		assert(m_activeInputDelegate);
		m_activeInputDelegate->DefineChordInput();
		assert(m_globalGameInputDelegate);
		m_globalGameInputDelegate->DefineChordInput();
	}
	else
	{
		//single key input
		assert(m_activeInputDelegate);
		m_activeInputDelegate->DefineHeldInput();
		assert(m_globalGameInputDelegate);
		m_globalGameInputDelegate->DefineHeldInput();
	}
}

//////////////////////////////////////////////////////////
// return - whether we have successfully consumed the given keyChord
bool const InputCore::TryKeyChord
(
	KeyChordPair _keyChord
)
{
	//check chord we're trying is well formed
	assert(_keyChord.first != SDL_SCANCODE_UNKNOWN
		&& _keyChord.second != SDL_SCANCODE_UNKNOWN);

	if (!m_activeChordProcessed) //@Revisit - logic's a bit weird as in seems like m_activeChordProcessed is being overloaded to also contain info as to whether we currently have an active chord or not.
	{
		if (m_activeChord == _keyChord)
		{
			m_activeChordProcessed = true;

			return true;
		}
	}

	return false; //no waiting chord matches given chord
}

#ifdef _DEBUG
bool const InputCore::InputBatchEnabled
(
) const
{
	return m_inputBatchEnabled;
}

void InputCore::BatchProcessInputs
(
	SDL_Event const _e
)
{
	if (_e.key.keysym.scancode == SDL_SCANCODE_I) //@Cleanup - cleaner solution where Chords are immune from batching. 
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
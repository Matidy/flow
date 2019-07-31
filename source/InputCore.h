#include <unordered_map>
#include <SDL_events.h>
#include <SDL_scancode.h>
#include "Interfaces/InputCoreIF.h"

class InputDelegate;

class InputCore : public InputCoreIF
{
public:
	//from InputCoreIF
	virtual void SetActiveInputDelegate(InputDelegate * _inputDelegate) override final;
	virtual void SetGlobalGameInputDelegate(InputDelegate * _inputDelegate) override final;
	virtual bool const& IsPressed(SDL_Scancode const _keyScanCode) override final;
	virtual bool const TryKeyChord(KeyChordPair _keyChord) override final;
	virtual bool const& ToggleInputBatching() override final;

	InputCore();

	void UpdateKeyboardState(SDL_Event const& _e);
	void CheckHeldKeyboardInput();

#ifdef _DEBUG
	bool const InputBatchEnabled() const;
	void BatchProcessInputs(SDL_Event const _e);
#endif

private:
	//Input Delegate who's Input functions are currently active
	InputDelegate * m_activeInputDelegate;
	//NOTE: should only be used/set to GameData object
	InputDelegate * m_globalGameInputDelegate;

	//Keyboard Pressed State Requirements
	//1. Continuous frame by frame code activation for pressed keys (e.g. update movement direction)
	//stores which keys are currently being held down
	std::unordered_map<SDL_Scancode, bool> m_pressedKeys;

	//2. Single fire event for two keys held down at the same time
	//pair of keys forming our current active chord,
	//first == SDL_SCANCODE_UNKNOWN - no active chord
	//second == SDL_SCANCODE_UNKNOWN - waiting on a second key press to form a completed chord
	KeyChordPair m_activeChord;
	//used to make sure we only process a chord input once per chord completion. Will wait until
	//chord second is released to be able to complete another chord or until first is released to
	//signal we're no longer waiting for a chord to be completed.
	bool m_activeChordProcessed;
	

	bool const ActiveChordWaiting() { return m_activeChord.first != SDL_SCANCODE_UNKNOWN; }

#ifdef _DEBUG
	static constexpr Uint8 m_inputBatchSize = 10u;
	SDL_Event m_inputBatch[m_inputBatchSize];
	Uint8 m_currInputBatchWaiting;
	bool m_inputBatchEnabled;
#endif 
};

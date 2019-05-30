#include <unordered_map>
#include <SDL_scancode.h>

class WorldGrid;
union SDL_Event;
#if _DEBUG
class Debug;
#endif

typedef std::pair<SDL_Scancode, SDL_Scancode> KeyChordPair;


class Input
{
public:
	Input();
	bool Init(WorldGrid * const _worldGrid
#if _DEBUG	
		, Debug * const _debug
#endif
			 );

	void UpdateKeyboardState(SDL_Event const& _e);
	void HandleInput();

#if _DEBUG
	bool const InputBatchEnabled() const;
	void BatchProcessInputs(SDL_Event const _e);
#endif

private:
	WorldGrid * m_worldGrid; //should this be read only? a- no we toggle debug bool from this class

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

	void KeyPressedEvent(SDL_Scancode const& _key);
	void KeyReleasedEvent(SDL_Scancode const& _key);

	bool const& IsPressed(SDL_Scancode const _keyScanCode);
	bool const TryKeyChord(KeyChordPair _keyChord);
	bool const ActiveChordWaiting() { return m_activeChord.first != SDL_SCANCODE_UNKNOWN; }

	void HandleGameInput();

#if _DEBUG
	Debug * m_debug;

	static constexpr Uint8 m_inputBatchSize = 10u;
	SDL_Event m_inputBatch[m_inputBatchSize];
	Uint8 m_currInputBatchWaiting;
	bool m_inputBatchEnabled;

	void HandleDebugInput();
#endif 
};

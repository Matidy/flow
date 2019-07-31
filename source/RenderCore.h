#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <SDL_pixels.h>
#include <SDL_rect.h>

#include "Interfaces/RenderCoreIF.h"

//forward declares
struct SDL_Window;
struct SDL_Surface;
struct SDL_Renderer;

class GameData;
class WorldGrid;
#ifdef _DEBUG
class Debug;
#endif

class RenderCore : public RenderCoreIF
{
  public:
	//from RenderCoreIF
	  virtual void CreateFadeLabel(char const * _symbol, char const * _text, SDL_Color _colour, SDL_Rect _displayRect, int32_t _timeBeforeFade) override final;

	//SDL data structures
	SDL_Window* m_gWindow;
	SDL_Renderer* m_gRenderer;

	//Window dimension constant
	static constexpr uint32_t WINDOW_WIDTH = 1024u;
	static constexpr uint32_t WINDOW_HEIGHT = (WINDOW_WIDTH/16)*9;

	RenderCore(void);
	~RenderCore();
	//intialise SDL
	bool Init();

	/**
	* By default, redrawing entire frame each frame to avoid added complexity in figuring out
	* which parts of the screen need redrawing and method to redraw those parts given their context.
	*/
	void DrawUpdate(GameData const& _gameData, uint32_t const _timeStep);

  private:
	  //could replace this with a screen class for each specific render context? Not sure what the data spec
	  //for that would be right now though
	  enum class WindowDrawState
	  {
			WDS_Game
#ifdef _DEBUG
			, WDS_DebugPropagation
			, WDS_DebugTessalation
#endif
	  };
	  WindowDrawState m_drawState;

	  //could use a switch draw context function to clear screen and update m_drawState here

	void DrawGame(WorldGrid const& _worldGrid);
	void ProcessTextLabels(uint32_t const _timeStep);

#ifdef _DEBUG
	// @TODO - better way to do this might be to have window store an array of function pointers to
	// debug draw functions defined in other classes (so that the debug function are local to the
	// data they're drawing from)
	void DrawDebugPropagation(Debug const& _debug);
	void DrawDebugTessalation(Debug const& _debug);
#endif

	struct FadingTextLabel
	{
		std::string m_symbol;
		std::string m_text; //would rather use a char const *, but itertor erase method seems to need to be able to use assignment operator, meaning this being const would be an issue.
		SDL_Color m_colour;
		SDL_Rect m_displayRect;
		int32_t m_timeBeforeFade;
		float const m_fadeDuration = 1500.f; //time in milliseconds
		float const m_fadePerMilliSecond = 255.f/m_fadeDuration; //assume full alpha on all labels for now

		FadingTextLabel(char const * _symbol, char const * _text, SDL_Color _colour, SDL_Rect _displayRect, uint32_t const _timeBeforeFade = 1500)
			: m_symbol(_symbol),
			  m_text(_text), 
			  m_colour(_colour),
			  m_displayRect(_displayRect),
			  m_timeBeforeFade(_timeBeforeFade)
		{}

		bool const ProcessFade(uint32_t const _timeStep);

		//const memeber variables mean need to explictly define assignment operator
		FadingTextLabel& operator=(const FadingTextLabel& _textLabel)
		{
			m_symbol = _textLabel.m_symbol;
			m_text = _textLabel.m_text;
			m_colour = _textLabel.m_colour;
			m_displayRect = _textLabel.m_displayRect;
			m_timeBeforeFade = _textLabel.m_timeBeforeFade;

			return *this;
		}

	private:
		//used to allow decrments to label alpha to be collected across multiple frames to avoid 
		//rounding issues, as alpha values are stored as ints
		float m_fadeAccumulator = 0.f; 
	};
	std::vector<FadingTextLabel> m_textLabels; 
};
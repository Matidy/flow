#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <SDL_pixels.h>
#include <SDL_rect.h>

#include "Globals.h"
#include "Interfaces/RenderCoreIF.h"

//forward declares
struct SDL_Window;
struct SDL_Surface;
struct SDL_Renderer;

class RenderDelegate;
class WorldGrid;
#ifdef _DEBUG
class Debug;
#endif

class RenderCore : public RenderCoreIF
{
  public:
	//from RenderCoreIF
	virtual void AddRenderDelegate(RenderDelegate const * _renderDelegate) override final;
	virtual void RemoveRenderDelegate(RenderDelegate const * _renderDelegate) override final;
	virtual void MaintainLabel(char const* _symbol, char const* _text, SDL_Color _colour, SDL_Rect _displayRect) override final;
	virtual bool RemoveLabel(char const* _symbol) override final;
	virtual void MaintainFadeLabel(char const * _symbol, char const * _text, SDL_Color _colour, SDL_Rect _displayRect, int32_t _timeBeforeFade) override final;

	//SDL data structures
	SDL_Window* m_gWindow;
	SDL_Renderer* m_gRenderer;

	RenderCore(void);
	~RenderCore();
	//intialise SDL
	bool Init();

	/**
	* By default, redrawing entire frame each frame to avoid added complexity in figuring out
	* which parts of the screen need redrawing and method to redraw those parts given their context.
	*/
	void DrawUpdate(uint32_t const _timeStep);

  private:
	std::vector<RenderDelegate const *> m_renderDelegates;
	
	void ProcessTextLabels(uint32_t const _timeStep);
	bool const StrMatch(char const* _str1, char const* _str2);

	struct TextLabel
	{
		std::string m_symbol;
		std::string m_text; //would rather use a char const *, but itertor erase method seems to need to be able to use assignment operator, meaning this being const would be an issue.
		SDL_Color m_colour;
		SDL_Rect m_displayRect;

		TextLabel(char const* _symbol, char const* _text, SDL_Color _colour, SDL_Rect _displayRect, bool _fadeLabel = false)
			: m_symbol(_symbol),
			  m_text(_text),
			  m_colour(_colour),
			  m_displayRect(_displayRect)
		{}
	};
	struct FadingTextLabel :
		public TextLabel
	{
		int32_t m_timeBeforeFade;
		float const m_fadeDuration = 1500.f; //time in milliseconds
		float const m_fadePerMilliSecond = 255.f/m_fadeDuration; //assume full alpha on all labels for now

		FadingTextLabel(char const* _symbol, char const* _text, SDL_Color _colour, SDL_Rect _displayRect, uint32_t const _timeBeforeFade = 1500)
			: TextLabel(_symbol, _text, _colour, _displayRect),
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
	std::vector<TextLabel>		  m_textLabels;		//non-fade labels
	std::vector<FadingTextLabel>  m_textFadeLabels; 
};

//SDL render API
/*
	SDL_SetRenderDrawColor
	SDL_RenderFillRect(m_gRenderer,
	SDL_RenderDrawRect(m_gRenderer,
	SDL_RenderDrawLines
	SDL_RenderDrawLine
	SDL_RenderDrawPoints
	SDL_RenderDrawPoint

	SDL_RenderClear

	SDL_RenderSetViewport - specify only drawing to a sub-region of window/surface/renderer

	SDL_BlitSurface( gHelloWorld, NULL, gScreenSurface, NULL );
*/
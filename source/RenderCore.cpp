#include "RenderCore.h"

#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_ttf.h>

#include "Data/ValRGBA.h"
#include "GameData.h"

//brief - checks if two strings are an exact match, case sensitive, no sub-string handling.
//return - true for exact match, false for any difference.
bool const StrMatch(char const * _str1, char const * _str2)
{
	for (uint32_t i = 0; ; i++)
	{
		if (_str1[i] == '\0')
		{
			if (_str2[i] == '\0')
			{
				return true; //strings match
			}
			else
			{
				return false; //_str2 has identical begininning to _str1, but has more characters at end.
			}
		}
		else if (_str1[i] == _str2[i])
		{
			continue;
		}
		else
		{
			return false;
		}
	}
}

void RenderCore::CreateFadeLabel
(
	char const * _symbol,
	char const * _text,
	SDL_Color _colour,
	SDL_Rect _displayRect,
	int32_t _timeBeforeFade
)
{
	for (FadingTextLabel& _textLabel : m_textLabels)
	{
		bool labelSymbolAlreadyExists = StrMatch(_textLabel.m_symbol.data(), _symbol);
		if (labelSymbolAlreadyExists)
		{
			_textLabel.m_text = _text;
			_textLabel.m_colour = _colour;
			_textLabel.m_displayRect = _displayRect;
			_textLabel.m_timeBeforeFade = _timeBeforeFade;

			return;
		}
	}
	
	FadingTextLabel newLabel(_symbol, _text, _colour, _displayRect, _timeBeforeFade);
	m_textLabels.push_back(newLabel);
	
	return;
}


RenderCore::RenderCore(void) 
	:
	m_gWindow(nullptr),
	m_gRenderer(nullptr),
	m_drawState(WindowDrawState::WDS_Game)
{
}

RenderCore::~RenderCore()
{
	//@TODO - feeling there was a reason initial SDL setup had a close method and didn't just do this
	//        in the destructor - should re-visit setting up SDL tutorial to double check.
	//Destroy window
	SDL_DestroyWindow(m_gWindow);
	m_gWindow = nullptr;
}

bool RenderCore::Init() 
{
	bool success = false;

	//Create window
	m_gWindow = SDL_CreateWindow("flow", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if(m_gWindow == nullptr) 
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}
	else 
	{
		//Create renderer for window
		m_gRenderer = SDL_CreateRenderer(m_gWindow, -1, SDL_RENDERER_ACCELERATED);
		if (m_gRenderer == nullptr) 
		{
			printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		}
		else 
		{
			success = true;
		}
	}

	// just code testing this function
	SDL_RendererInfo rendererInfo;
	if (SDL_GetRenderDriverInfo(0, &rendererInfo) < 0)
	{
		printf("Failed to get Renderer Driver Info");
	}

	return success;
}

void RenderCore::DrawUpdate(GameData const& _gameData, uint32_t _timeStep)
{
#ifdef _DEBUG
	if (_gameData.m_debug.DebugEnabled())
	{
		if (_gameData.m_debug.DisplayTessalation())
		{
			// tess debug draw
			if (m_drawState != WindowDrawState::WDS_DebugTessalation)
			{
				SDL_SetRenderDrawColor(m_gRenderer, 0, 0, 0, 255);
				SDL_RenderClear(m_gRenderer);

				m_drawState = WindowDrawState::WDS_DebugTessalation;
			}

			DrawDebugTessalation(_gameData.m_debug);
		}
		else
		{
			// prop debug draw
			if (m_drawState != WindowDrawState::WDS_DebugPropagation)
			{
				SDL_SetRenderDrawColor(m_gRenderer, 0, 0, 0, 255);
				SDL_RenderClear(m_gRenderer);

				m_drawState = WindowDrawState::WDS_DebugPropagation;
			}

			DrawDebugPropagation(_gameData.m_debug);
		}
	}
	else
	{
#endif
		if (m_drawState != WindowDrawState::WDS_Game)
		{
			SDL_SetRenderDrawColor(m_gRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_gRenderer);

			m_drawState = WindowDrawState::WDS_Game;
		}

		DrawGame(_gameData.m_worldGrid);
#ifdef _DEBUG
	}
#endif

	ProcessTextLabels(_timeStep);

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
}

void RenderCore::DrawGame
(
	WorldGrid const& _worldGrid
)
{

}

void RenderCore::ProcessTextLabels(uint32_t const _timeStep)
{
	TTF_Font* raleway = TTF_OpenFont("../SDL2_ttf-2.0.15/external/fonts/raleway/Raleway-Regular.ttf", 24); //@TODO - need to decide if font files should be included in working directory to avoid releative path issues like this.

	std::vector<RenderCore::FadingTextLabel>::iterator iter = m_textLabels.begin();
	for (; iter != m_textLabels.end();)
	{
		RenderCore::FadingTextLabel& textLabel = *iter;
		SDL_Surface* labelSurface = TTF_RenderText_Blended(raleway, textLabel.m_text.data(), textLabel.m_colour);
		SDL_Texture* labelTexture = SDL_CreateTextureFromSurface(m_gRenderer, labelSurface);
		SDL_FreeSurface(labelSurface);
		labelSurface = nullptr;

		SDL_RenderCopy(m_gRenderer, labelTexture, nullptr, &textLabel.m_displayRect);

		//process fade, i.e. alpha value changes for our labels. Labels removed if alpha == 0.
		if (textLabel.ProcessFade(_timeStep))
		{
			//@consider - due to destructive nature of erase, better way to do loops like this might be to have some 'tag for removal' feature, and then to do one array pass/rearrangment at the end that removes all the tagged entries.
			iter = m_textLabels.erase(iter); 
		}
		else
		{
			++iter;
		}
	}

	TTF_CloseFont(raleway);
}

#ifdef _DEBUG //debug function section
void RenderCore::DrawDebugPropagation(Debug const& _debug)
{
	flPoint const * const rDebugWorldData = _debug.GetDebugWorldDataRead();
	if(rDebugWorldData)
	{
		uint32_t verticleBuffer = 128u;
		uint32_t debugWorldWidth = RenderCore::WINDOW_HEIGHT - (2 * verticleBuffer);
		uint32_t tileWidth = debugWorldWidth / Debug::m_debugWorldDim; //this line rounds down from 18.82 to 18
		uint32_t horizontalBuffer = (RenderCore::WINDOW_WIDTH - debugWorldWidth) / 2;

		ValRGBA maxColour;
		maxColour.r = 255;
		maxColour.g = 140;
		maxColour.b = 0;
		maxColour.a = 255;
		ValRGBA minColour;
		minColour.r = 96;
		minColour.g = 53;
		minColour.b = 0;
		minColour.a = 255;
		uint32_t saturationVal = 16u;
		ValRGBA colourIncrement = (maxColour - minColour)/saturationVal;
		
		ValRGBA curColour;
		uint32_t x = horizontalBuffer;
		uint32_t y = verticleBuffer;
		for (uint32_t i=0; i < Debug::m_debugWorldDim*Debug::m_debugWorldDim; i++)
		{
			SDL_Rect rect;
			rect.x = x;
			rect.y = y;
			rect.w = tileWidth;
			rect.h = tileWidth;

			curColour = colourIncrement * rDebugWorldData[i].m_energy; //@robustness - Will overflow for vals > 16
			SDL_SetRenderDrawColor(m_gRenderer, curColour.r, curColour.g, curColour.b, curColour.a);
			SDL_RenderFillRect(m_gRenderer, &rect); //@check - passing in a local ref, is c++ smart enough to preserve the allocation? a - works, though might be due to SDL function copying data from passed in reference object into render buffer

			if (x < horizontalBuffer + (Debug::m_debugWorldDim-1)*tileWidth)
			{
				x += tileWidth;
			}
			else
			{
				//start new row
				x = horizontalBuffer;
				y += tileWidth;
			}
		}
		
	}
}

void RenderCore::DrawDebugTessalation
(
	Debug const& _debug
)
{
	SDL_Point points[3000];
	Uint32 i = 0;
	for (Uint32 x = 0; x <= RenderCore::WINDOW_WIDTH;)
	{
		for (Uint32 y = 0; y <= RenderCore::WINDOW_HEIGHT;)
		{
			Uint32 offset = 0;
			if (y / 16 & 1) //checking for odd row through Least Sig Bit
			{
				offset = 8;
			}

			points[i].x = x + offset;
			points[i].y = y;

			i++;
			y += 16;
		}

		x += 16;
	}

	SDL_SetRenderDrawColor(m_gRenderer, 255, 255, 255, 255);
	//SDL_RenderDrawLines(m_gRenderer, points, i);
	SDL_RenderDrawPoints(m_gRenderer, points, i);
}
#endif //end debug function section

////////////////////////////////////////////////////////////////////////////
// return - true for fade completed, false for not completed.
bool const RenderCore::FadingTextLabel::ProcessFade
(
	uint32_t _timeStep
)
{
	if (m_timeBeforeFade > 0)
		m_timeBeforeFade -= _timeStep;
	else
	{
		m_fadeAccumulator += m_fadePerMilliSecond * static_cast<float>(_timeStep);
		if (m_fadeAccumulator >= 1.f)
		{
			//transfer whole num value from accumulator to decrementVal for processing
			uint32_t decrementVal = static_cast<uint32_t>(floor(m_fadeAccumulator));
			m_fadeAccumulator -= floor(m_fadeAccumulator);

			if (m_colour.a > decrementVal)
				m_colour.a -= decrementVal;
			else
			{
				m_colour.a = 0;
				return true; //true for fade complete
			}
		}
		
	}

	return false; //false for fade on-going
}
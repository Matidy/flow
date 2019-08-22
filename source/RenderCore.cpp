#include "RenderCore.h"

#include <iostream>

#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_ttf.h>

#include "RenderDelegate.h"
#include "Data/ValRGBA.h"
#include "GameData.h"

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from RenderCoreIF   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void RenderCore::AddRenderDelegate(RenderDelegate const * _renderDelegate)
{
	assert(_renderDelegate);
	m_renderDelegates.push_back(_renderDelegate);
}

//brief - called when RenderDelegate is Destructed to make sure RenderCore clears what is soon to become a dangling pointer
void RenderCore::RemoveRenderDelegate(RenderDelegate const * _renderDelegate)
{
	assert(_renderDelegate);
	std::vector<RenderDelegate const *>::iterator iter = m_renderDelegates.begin();
	std::vector<RenderDelegate const *>::iterator end = m_renderDelegates.end();
	for (; iter != end; ++iter)
	{
		RenderDelegate const * rd = *iter;
		if (rd == _renderDelegate) //@check - using object address to ID, is this valid/safe? If yeah, expand to InputDelegate
		{
			m_renderDelegates.erase(iter);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
RenderCore::RenderCore(void) 
	:
	m_gWindow(nullptr),
	m_gRenderer(nullptr)
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
	m_gWindow = SDL_CreateWindow("flow", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Globals::WINDOW_WIDTH, Globals::WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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
	//call draw methods on enabled delegates
	for (RenderDelegate const * rd : m_renderDelegates)
	{
		assert(rd);
		if (rd->Enabled())
		{
			rd->DelegateDraw(m_gRenderer);
		}
	}

	ProcessTextLabels(_timeStep);
}

//brief - draw fade labels to backbuffer, process fade and remove labels that have finished their fade from m_textLabels
void RenderCore::ProcessTextLabels(uint32_t const _timeStep)
{
	TTF_Font* raleway = TTF_OpenFont("../SDL2_ttf-2.0.15/external/fonts/raleway/Raleway-Regular.ttf", 24); //@TODO - need to decide if font files should be included in working directory to avoid releative path issues like this.
	
	std::vector<RenderCore::FadingTextLabel>::iterator iter = m_textLabels.begin();
	for (; iter != m_textLabels.end();)
	{
		if (raleway)
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
		else
		{
			std::cout << "Missing font when trying to draw TextLabels." << std::endl;
			m_textLabels.clear();
			break;
		}
	}

	TTF_CloseFont(raleway);
}

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

////////////////////////////////////////////////////////////////////////////
// return - true for fade completed, false for ongoing.
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
			uint32_t decrementVal = static_cast<uint32_t>(floorf(m_fadeAccumulator));
			m_fadeAccumulator -= floorf(m_fadeAccumulator);

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
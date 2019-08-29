#pragma once
#include <SDL_pixels.h>
#include <SDL_rect.h>

class RenderDelegate;
struct FadingTextLabel;

//@consider - rather than allowing all RenderCore functions through this interface, could pass an object into RenderDelegate's
//			  draw function that provided exlusive access to draw functions. This would mean only that function is able to 
//			  affect what is drawn on screen, creating cleaner code.
struct RenderCoreIF
{
	virtual void AddRenderDelegate(RenderDelegate const * _renderDelegate) = 0;
	virtual void RemoveRenderDelegate(RenderDelegate const * _renderDelegate) = 0;

	virtual void MaintainLabel(char const* _symbol, char const* _text, SDL_Color _colour, SDL_Rect _displayRect) = 0;
	//return - true for label removed, false for not found/removed
	virtual bool RemoveLabel(char const* _symbol) = 0;
	virtual void MaintainFadeLabel(char const * _symbol, char const * _text, SDL_Color _colour, SDL_Rect _displayRect, int32_t _timeBeforeFade) = 0;
};
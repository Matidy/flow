#pragma once

struct FadingTextLabel;

struct RenderCoreIF
{
	virtual void CreateFadeLabel(char const * _symbol, char const * _text, SDL_Color _colour, SDL_Rect _displayRect, int32_t _timeBeforeFade) = 0;
};
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include "Interfaces/InputCoreIF.h"
#include "Interfaces/RenderCoreIF.h"

#include "GameData.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
GameData::GameData(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF)
	: InputDelegate(_inputCoreIF)
#ifdef _DEBUG
	, m_debug(_renderCoreIF, _inputCoreIF)
#endif
	, m_worldGrid(_renderCoreIF, _inputCoreIF)
	, m_renderCoreIF(_renderCoreIF)
{
	//should be the only call to this
	m_inputCoreIF.SetGlobalGameInputDelegate(this);

	//default Active Input Delegate
	m_inputCoreIF.SetActiveInputDelegate(&m_worldGrid);
	m_worldGrid.GenerateWorld();
}

void GameData::UpdateStep(uint32_t const _timeStep)
{
#ifdef _DEBUG
	if (m_debug.DebugEnabled())
	{
		m_debug.UpdateStep(_timeStep);
	}
	else
	{
#endif		
		m_worldGrid.UpdateStep(_timeStep);
#ifdef _DEBUG
	}
#endif
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void GameData::DefineChordInput()
{
#ifdef _DEBUG
	//handle switching between Debug and WorldGrid contexts
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_D)))
	{
		bool const& debugEnabled = m_debug.ToggleDebug();
		if (debugEnabled)
		{
			m_inputCoreIF.SetActiveInputDelegate(&m_debug);

			//turn off world render, turn on debug render
			m_worldGrid.ToggleRender(false);
			m_debug.ToggleRender(true);
		}
		else
		{
			m_inputCoreIF.SetActiveInputDelegate(&m_worldGrid);

			//turn off debug render, turn on world render
			m_debug.ToggleRender(false);
			m_worldGrid.ToggleRender(true);
		}
	}

	//global batch input chord
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_I)))
	{
		bool const& inputBatchingEnabled = m_inputCoreIF.ToggleInputBatching();

		SDL_Color labelColour{ 255, 255, 255, 255 };
		SDL_Rect labelRect;
		labelRect.x = 8;
		labelRect.y = 8;
		labelRect.w = 196;
		labelRect.h = 32;
		if (inputBatchingEnabled)
		{
			m_renderCoreIF.CreateFadeLabel("Debug Toggle", "Input Batch Enabled", labelColour, labelRect, 1500);
		}
		else
		{
			m_renderCoreIF.CreateFadeLabel("Debug Toggle", "Input Batch Disabled", labelColour, labelRect, 1500);
		}
	}
#endif
}
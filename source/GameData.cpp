#include <string>
#include <iostream>

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

	//frame count label
	m_frameCounter.m_frameAverageTimestep = m_frameCounter.m_frameAverageTimestep*0.99f + static_cast<float>(_timeStep)*0.01f;
	m_frameCounter.m_timeSinceLastUpdate += _timeStep;
	if (m_frameCounter.m_display)
	{
		if (m_frameCounter.m_timeSinceLastUpdate >= m_frameCounter.m_timeBetweenUpdates)
		{
			m_frameCounter.m_timeSinceLastUpdate = 0u;
			float frameCount = static_cast<float>(1000u)/static_cast<float>(m_frameCounter.m_frameAverageTimestep);
			
			std::string labelText = { 'F', 'P', 'S', ':', ' ' };
			std::string frameCountString = std::to_string(frameCount);
			for (uint32_t i = 0; frameCountString[i] != '\0'; i++)
			{
				char& digit = frameCountString[i];
				if (digit == '.')
				{
					i += 2; //to first decimal
					frameCountString.resize(i);
					break; 
				}
			}
			labelText.append(frameCountString.data());

			SDL_Color labelColour;
			labelColour.r = 255u;
			labelColour.g = 255u;
			labelColour.b = 255u;
			labelColour.a = 255u;

			SDL_Rect displayRect;
			displayRect.w = 96;
			displayRect.h = 24;
			displayRect.x = Globals::WINDOW_WIDTH - displayRect.w - 8;
			displayRect.y = 0;

			m_renderCoreIF.MaintainLabel("Frame Count", labelText.data(), labelColour, displayRect);
		}
	}
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void GameData::DefineChordInput(uint32_t _timeStep) //this is for global InputDelegate (always active)
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

	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_F)))
	{
		bool& displayFrameCounter = m_frameCounter.m_display;
		displayFrameCounter = !displayFrameCounter;
		if (!displayFrameCounter)
		{
			m_renderCoreIF.RemoveLabel("Frame Count");
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
			m_renderCoreIF.MaintainFadeLabel("Debug Toggle", "Input Batch Enabled", labelColour, labelRect, 1500);
		}
		else
		{
			m_renderCoreIF.MaintainFadeLabel("Debug Toggle", "Input Batch Disabled", labelColour, labelRect, 1500);
		}
	}
}
#endif

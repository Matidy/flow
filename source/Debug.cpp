#include <string>
#include <stdio.h>
#include <assert.h>

#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_render.h>

#include "Debug.h"
#include "Globals.h"

#include "Interfaces/InputCoreIF.h"
#include "Interfaces/RenderCoreIF.h"

#include "Data/ValRGBA.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
Debug::Debug(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF)
	: RenderDelegate(_renderCoreIF, false),
	  InputDelegate(_inputCoreIF),
	  m_debugEnabled(false),
	  m_propagationRate(2.0f),
	  m_propagationMode(PropagationMode::SimpleDirectional),
	  m_drawMode(DrawMode::Propagation)
{
	m_debugWorld = new flPoint[m_debugWorldSize];

	uint32_t centerIndex = m_debugWorldSize % 2 ? m_debugWorldSize/2 + 1 : m_debugWorldSize/2;
	m_debugWorld[centerIndex].m_energy = 1;
	m_debugWorld[centerIndex].m_direction = flPoint::Direction::ALL;

	m_pointsToPropagate.push_back(centerIndex);

	m_tessalationPoints = new SDL_Point[3000];

	//@TODO - would be good to have a debug mode where we can cycle through different resolutions of grid to get a real feel
	//		  for each resolution.
}

Debug::~Debug()
{
	//@bug - on window close - HEAP CORRUPTION DETECTED: before Normal block (#166) at 0x1DFB4060.
	//CRT detected that the application wrote to memory before start of heap buffer.
	//cause - propagate debug grid 8 or more times to get to occur, on 8th & 9th propagation only row on left
	//		  gets propagated.
	//action - going to leave for now as expect to rewrite prop code at later date
	delete[] m_debugWorld;
	delete[] m_tessalationPoints;
}

void Debug::UpdateStep(uint32_t const _timeStep)
{

}

bool const& Debug::ToggleDebug()
{
	m_debugEnabled = !m_debugEnabled;

	SDL_Color displayColour{ 255, 255, 255, 255 };
	SDL_Rect displayRect;
	displayRect.x = 8;
	displayRect.y = 8;
	displayRect.w = 160;
	displayRect.h = 32;
	if (m_debugEnabled)
	{
		m_renderCoreIF.MaintainFadeLabel("Debug Toggle", "Debug Enabled", displayColour, displayRect, 1500);
	}
	else
	{
		m_renderCoreIF.MaintainFadeLabel("Debug Toggle", "Debug Disabled", displayColour, displayRect, 1500);
	}

	return m_debugEnabled;
}

bool const Debug::DebugEnabled() const
{
	return m_debugEnabled;
}

void Debug::CyclePropModeLeft()
{
	if (m_propagationMode == 0)
	{
		m_propagationMode = (uint32_t)PropagationMode::CountPropagationMode - 1;
	}
	else
	{
		m_propagationMode -= 1;
	}

}

void Debug::CyclePropModeRight()
{
	if (m_propagationMode == (uint32_t)PropagationMode::CountPropagationMode - 1)
	{
		m_propagationMode = 0;
	}
	else
	{
		m_propagationMode++;
	}
}

void Debug::PropagateAdjacent()
{
	std::vector<uint32_t> nextFramePoints;
	std::vector<uint32_t>::const_iterator iter = m_pointsToPropagate.cbegin();
	std::vector<uint32_t>::const_iterator end = m_pointsToPropagate.cend();
	switch (m_propagationMode)
	{
	case PropagationMode::SimpleDirectional:
	{
		for (; iter != end; ++iter)
		{
			uint32_t index = *iter;
			flPoint& curPoint = m_debugWorld[index];

			switch (curPoint.m_direction)
			{
			case flPoint::Direction::ALL:
			{
				uint32_t iUp = GetIndexUp(index);
				flPoint& upPoint = m_debugWorld[iUp];
				upPoint.m_direction = flPoint::Direction::UP;
				upPoint.m_energy += 1;
				nextFramePoints.push_back(iUp);

				uint32_t iLeft = GetIndexLeft(index);
				flPoint& leftPoint = m_debugWorld[iLeft];
				leftPoint.m_direction = flPoint::Direction::LEFT;
				leftPoint.m_energy += 1;
				nextFramePoints.push_back(iLeft);

				uint32_t iDown = GetIndexDown(index);
				flPoint& downPoint = m_debugWorld[iDown];
				downPoint.m_direction = flPoint::Direction::DOWN;
				downPoint.m_energy += 1;
				nextFramePoints.push_back(iDown);

				uint32_t iRight = GetIndexRight(index);
				flPoint& rightPoint = m_debugWorld[iRight];
				rightPoint.m_direction = flPoint::Direction::RIGHT;
				rightPoint.m_energy += 1;
				nextFramePoints.push_back(iRight);

				break;
			}
			case flPoint::Direction::UP:
			{
				uint32_t iUp = GetIndexUp(index);
				flPoint& upPoint = m_debugWorld[iUp];
				upPoint.m_direction = flPoint::Direction::UP;
				upPoint.m_energy += 1;
				nextFramePoints.push_back(iUp);

				break;
			}
			case flPoint::Direction::LEFT:
			{
				uint32_t iLeft = GetIndexLeft(index);
				flPoint& leftPoint = m_debugWorld[iLeft];
				leftPoint.m_direction = flPoint::Direction::LEFT;
				leftPoint.m_energy += 1;
				nextFramePoints.push_back(iLeft);

				break;
			}
			case flPoint::Direction::DOWN:
			{
				uint32_t iDown = GetIndexDown(index);
				flPoint& downPoint = m_debugWorld[iDown];
				downPoint.m_direction = flPoint::Direction::DOWN;
				downPoint.m_energy += 1;
				nextFramePoints.push_back(iDown);

				break;
			}
			case flPoint::Direction::RIGHT:
			{
				uint32_t iRight = GetIndexRight(index);
				flPoint& rightPoint = m_debugWorld[iRight];
				rightPoint.m_direction = flPoint::Direction::RIGHT;
				rightPoint.m_energy += 1;
				nextFramePoints.push_back(iRight);

				break;
			}
			}
		}

		break;
	}
	case PropagationMode::SemiCircleDirectional:
	{
		for (; iter != end; ++iter)
		{
			uint32_t index = *iter;
			flPoint& curPoint = m_debugWorld[index];
			//get previously visited and visit adjacent
			//remove from list once processed
			//add adjacent to list
			//peter-out at world boundary (no-op)
			uint32_t iUp = GetIndexUp(index);
			uint32_t iDown = GetIndexDown(index);
			uint32_t iLeft = GetIndexLeft(index);
			uint32_t iRight = GetIndexRight(index);
			if (curPoint.m_direction != flPoint::Direction::DOWN)
			{
				if (iUp != m_nullIndex)
				{
					m_debugWorld[iUp].m_energy += 1;
					nextFramePoints.push_back(iUp);
				}
			}
			if (curPoint.m_direction != flPoint::Direction::UP)
			{
				if (iDown != m_nullIndex)
				{
					m_debugWorld[iDown].m_energy += 1;
					nextFramePoints.push_back(iDown);
				}
			}
			if (curPoint.m_direction != flPoint::Direction::RIGHT)
			{
				if (iLeft != m_nullIndex)
				{
					m_debugWorld[iLeft].m_energy += 1;
					nextFramePoints.push_back(iLeft);
				}
			}
			if (curPoint.m_direction != flPoint::Direction::LEFT)
			{
				if (iRight != m_nullIndex)
				{
					m_debugWorld[iRight].m_energy += 1;
					nextFramePoints.push_back(iRight);
				}
			}
		}

		break;
	}
	}

	m_pointsToPropagate = nextFramePoints;
}

void Debug::UnpropagateAdjacent()
{
	std::vector<uint32_t>::iterator iter = m_pointsToPropagate.begin();
	std::vector<uint32_t>::iterator end = m_pointsToPropagate.end();
	for (; iter != end;)
	{
		uint32_t index = *iter;
		int16_t& curEnergy = m_debugWorld[index].m_energy;
		curEnergy -= 1;
		assert(curEnergy >= 0);
		//if ()
		//@TODO - Need to figure out what it means to run the propagation in reverse
	}
}

void Debug::ResetWorldGrid()
{
	uint32_t centerIndex = m_debugWorldDim % 2 ? m_debugWorldDim / 2 + 1 : m_debugWorldDim / 2;
	for (uint16_t i = 0; i < m_debugWorldDim*m_debugWorldDim; i++)
	{
		if (i != centerIndex)
		{
			m_debugWorld[i].m_energy = 0;
		}
		else
		{
			m_debugWorld[i].m_energy = 1;
		}
	}
}

uint32_t Debug::GetIndexUp
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex < m_debugWorldDim)
	{
		//top row, no up to get
		return m_nullIndex;
	}
	else
	{
		return _currentPointIndex - m_debugWorldDim;
	}
}

uint32_t Debug::GetIndexDown(uint32_t _currentPointIndex)
{
	if (_currentPointIndex > (m_debugWorldDim*m_debugWorldDim) - 1 - m_debugWorldDim)
	{
		//bottom row, no down to get
		return m_nullIndex;
	}
	else
	{
		return _currentPointIndex + m_debugWorldDim;
	}
}

uint32_t Debug::GetIndexLeft(uint32_t _currentPointIndex)
{
	if (_currentPointIndex % m_debugWorldDim == 0)
	{
		//left most row, no left to get
		return m_nullIndex;
	}
	else
	{
		return _currentPointIndex - 1;
	}
}

uint32_t Debug::GetIndexRight(uint32_t _currentPointIndex)
{
	if (_currentPointIndex % m_debugWorldDim == m_debugWorldDim - 1)
	{
		//right most row, no right to get
		return m_nullIndex;
	}
	else
	{
		return _currentPointIndex + 1;
	}
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from RenderDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void Debug::DelegateDraw(SDL_Renderer * const _gRenderer) const
{
	switch (m_drawMode)
	{
	case DrawMode::Propagation:
	{
		uint32_t debugWorldDim = m_tilesDimPixels*m_debugWorldDim;
		uint32_t verticleBuffer = (Globals::WINDOW_HEIGHT - debugWorldDim) / 2;
		uint32_t horizontalBuffer = (Globals::WINDOW_WIDTH - debugWorldDim) / 2;

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
		ValRGBA colourIncrement = (maxColour - minColour) / saturationVal;

		ValRGBA curColour;
		SDL_Rect rect;
		rect.x = horizontalBuffer;
		rect.y = verticleBuffer;
		rect.w = m_tilesDimPixels;
		rect.h = m_tilesDimPixels;
		for (uint32_t i = 0; i < m_debugWorldSize; i++)
		{
			curColour = colourIncrement * m_debugWorld[i].m_energy*16; //@robustness - Will overflow for vals > 16
			SDL_SetRenderDrawColor(_gRenderer, curColour.r, curColour.g, curColour.b, curColour.a);
			SDL_RenderFillRect(_gRenderer, &rect); //@check - passing in a local ref, is c++ smart enough to preserve the allocation? a - works, though might be due to SDL function copying data from passed in reference object into render buffer

			if (rect.x + rect.w < horizontalBuffer + debugWorldDim)
			{
				rect.x += m_tilesDimPixels;
			}
			else
			{
				//start new row
				rect.x = horizontalBuffer;
				rect.y += m_tilesDimPixels;
			}
		}

		//draw grid lines over the top
		curColour = colourIncrement * 4;
		SDL_SetRenderDrawColor(_gRenderer, curColour.r, curColour.g, curColour.b, curColour.a);
		flVec2<int> p1;
		p1.x = horizontalBuffer;
		p1.y = verticleBuffer;
		flVec2<int> p2;
		p2.x = horizontalBuffer;
		p2.y = verticleBuffer + m_debugWorldDim * m_tilesDimPixels - 1;
		for (uint32_t i = 0; i< m_debugWorldDim; i++)
		{
			SDL_RenderDrawLine(_gRenderer, p1.x, p1.y, p2.x, p2.y);
			p1.x += m_tilesDimPixels;
			p2.x += m_tilesDimPixels;
		}

		p1.x = horizontalBuffer;
		p1.y = verticleBuffer;
		p2.x = horizontalBuffer + m_debugWorldDim * m_tilesDimPixels - 1;
		p2.y = verticleBuffer;
		for (uint32_t i = 0; i< m_debugWorldDim; i++)
		{
			SDL_RenderDrawLine(_gRenderer, p1.x, p1.y, p2.x, p2.y);
			p1.y += m_tilesDimPixels;
			p2.y += m_tilesDimPixels;
		}

		break;
	}
	case DrawMode::Tessalation:
	{
		Uint32 i = 0;
		for (Uint32 x = 0; x <= Globals::WINDOW_WIDTH;)
		{
			for (Uint32 y = 0; y <= Globals::WINDOW_HEIGHT;)
			{
				Uint32 offset = 0;
				if (y / 16 & 1) //checking for odd row through Least Sig Bit
				{
					offset = 8;
				}

				m_tessalationPoints[i].x = x + offset;
				m_tessalationPoints[i].y = y;

				i++;
				y += 16;
			}

			x += 16;
		}

		SDL_SetRenderDrawColor(_gRenderer, 255, 255, 255, 255);
		//SDL_RenderDrawLines(m_gRenderer, points, i);
		SDL_RenderDrawPoints(_gRenderer, m_tessalationPoints, i);

		break;
	}
	}
}

void Debug::MaintainMousePosLabel(flVec2<int>& _mousePos)
{
	std::string labelText("Pos X: ");
	labelText.append(std::to_string(_mousePos.x));
	labelText.append(", Pos Y: ");
	labelText.append(std::to_string(_mousePos.y));

	SDL_Color displayColour;
	displayColour.r = 255;
	displayColour.g = 255;
	displayColour.b = 255;
	displayColour.a = 255;

	SDL_Rect displayRect;
	displayRect.w = 128;
	displayRect.h = 24;
	displayRect.x = Globals::WINDOW_WIDTH - displayRect.w - 8;
	displayRect.y = 32;

	m_renderCoreIF.MaintainLabel("Mouse Position", labelText.data(), displayColour, displayRect);
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void Debug::DefineChordInput(uint32_t _timeStep)
{
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_M)))
	{
		m_showMousePos = !m_showMousePos;

		if (m_showMousePos)
		{
			flVec2<int> mousePos;
			m_inputCoreIF.GetMousePos(mousePos);
			MaintainMousePosLabel(mousePos);
		}
		else
		{
			m_renderCoreIF.RemoveLabel("Mouse Position");
		}
	}
}

void Debug::MouseMovementInput(flVec2<int> _mousePos)
{
	if (m_showMousePos)
	{
		MaintainMousePosLabel(_mousePos);
	}
}

void Debug::DefineHeldInput(uint32_t _timeStep)
{
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		printf("Left Pressed Debug Mode\n");
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		printf("Right Pressed Debug Mode\n");
	}
}

void Debug::KeyPressedInput(SDL_Scancode const& _key)
{
	if (_key == SDL_SCANCODE_T)
	{
		m_drawMode = DrawMode::Tessalation;
	}
	//cycle between propagation modes
	if (_key == SDL_SCANCODE_O)
	{
		CyclePropModeRight();
	}
	if (_key == SDL_SCANCODE_P)
	{
		CyclePropModeLeft();
	}
	// propagate debug control
	if (_key == SDL_SCANCODE_LEFT)
	{
		//UnpropagateAdjacent(); @Fix
	}
	if (_key == SDL_SCANCODE_RIGHT)
	{
		PropagateAdjacent();
	}
	if (_key == SDL_SCANCODE_SPACE)
	{

	}
}

void Debug::KeyReleasedInput(SDL_Scancode const& _key)
{
	if (_key == SDL_SCANCODE_T)
	{
		m_drawMode = DrawMode::Propagation;
	}
}

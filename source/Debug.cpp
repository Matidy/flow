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

	uint32_t centerIndex = m_debugWorldSize % 2 ? m_debugWorldSize/2 : m_debugWorldSize/2 - 1;
	m_debugWorld[centerIndex].m_energy = 1;
	m_debugWorld[centerIndex].m_direction = flPoint::Direction::ALL;

	m_pointsToPropagate.push_back(centerIndex);

	m_tessalationPoints = new SDL_Point[3000];

	m_vectors[0] = Vector2D<float>(200.f, 0.f, 300.f, 1200.f);
	m_vectors[1] = Vector2D<float>(0.f, 100.f, 1000.f, 200.f);

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
		//draw points as tiles
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
		rect.x = m_horizontalPixelBuffer;
		rect.y = m_verticlePixelBuffer;
		rect.w = m_tilesDimPixels;
		rect.h = m_tilesDimPixels;
		for (uint32_t i = 0; i < m_debugWorldSize; i++)
		{
			int16_t const& pointEnergy = m_debugWorld[i].m_energy;
			curColour = colourIncrement * pointEnergy * 16; //@robustness - Will overflow for vals > 16
			if (m_tileUnderMouseIndex == i)
			{
				curColour.r = 255;
				curColour.g = 255;
				curColour.b = 255;
				curColour.a = 255;
			}
			SDL_SetRenderDrawColor(_gRenderer, curColour.r, curColour.g, curColour.b, curColour.a);
			SDL_RenderFillRect(_gRenderer, &rect); //@check - passing in a local ref, is c++ smart enough to preserve the allocation? a - works, though might be due to SDL function copying data from passed in reference object into render buffer

			if (rect.x + rect.w < static_cast<int32_t>(m_horizontalPixelBuffer + m_debugWorldPixelDim))
			{
				rect.x += m_tilesDimPixels;
			}
			else
			{
				//start new row
				rect.x = m_horizontalPixelBuffer;
				rect.y += m_tilesDimPixels;
			}
		}

		//draw grid lines over the top
		curColour = colourIncrement * 4;
		SDL_SetRenderDrawColor(_gRenderer, curColour.r, curColour.g, curColour.b, curColour.a);
		//verticle lines
		flVec2<int> p1;
		p1.x = m_horizontalPixelBuffer;
		p1.y = m_verticlePixelBuffer;
		flVec2<int> p2;
		p2.x = m_horizontalPixelBuffer;
		p2.y = m_verticlePixelBuffer + m_debugWorldDim * m_tilesDimPixels - 1;
		for (uint32_t i = 0; i <= m_debugWorldDim; i++)
		{
			SDL_RenderDrawLine(_gRenderer, p1.x-1, p1.y, p2.x-1, p2.y);
			SDL_RenderDrawLine(_gRenderer, p1.x, p1.y, p2.x, p2.y);
			p1.x += m_tilesDimPixels;
			p2.x += m_tilesDimPixels;
		}

		//horizontal lines
		p1.x = m_horizontalPixelBuffer;
		p1.y = m_verticlePixelBuffer;
		p2.x = m_horizontalPixelBuffer + m_debugWorldDim * m_tilesDimPixels - 1;
		p2.y = m_verticlePixelBuffer;
		for (uint32_t i = 0; i <= m_debugWorldDim; i++)
		{
			SDL_RenderDrawLine(_gRenderer, p1.x, p1.y-1, p2.x, p2.y-1);
			SDL_RenderDrawLine(_gRenderer, p1.x, p1.y, p2.x, p2.y);
			p1.y += m_tilesDimPixels;
			p2.y += m_tilesDimPixels;
		}

#if 1
		//vectors
		SDL_SetRenderDrawColor(_gRenderer, 255, 255, 255, 255);
		for (uint8_t i = 0; i<m_vectorsSize; i++)
		{
			//using endpoint data
			Vector2D<float> const& vector = m_vectors[i];
			SDL_RenderDrawLine(_gRenderer,	static_cast<int>(vector.m_startingPoint.x),
											static_cast<int>(vector.m_startingPoint.y),
											static_cast<int>(vector.m_endPoint.x),
											static_cast<int>(vector.m_endPoint.y) );
			/*
			//using vector equation to get endpoint data
			flVec2<int> offset{ 150, 0 };
			flVec2<float> endPoint = vector.CalcEndpoint();
			SDL_RenderDrawLine(_gRenderer, offset.x + static_cast<int>(vector.m_startingPoint.x),
										   offset.y + static_cast<int>(vector.m_startingPoint.y),
										   offset.x + static_cast<int>(endPoint.x),
										   offset.y + static_cast<int>(endPoint.y) );
										   */
		}

		Vector2D<float> const& vector = m_vectors[0];
		flVec2<float> intersectionPoint = vector.GetIntersectionPoint(m_vectors[1]);
		SDL_SetRenderDrawColor(_gRenderer, 15, 255, 95, 255);
		rect.x = static_cast<int>(intersectionPoint.x)-1;
		rect.y = static_cast<int>(intersectionPoint.y)-1;
		rect.h = 3;
		rect.w = 3;
		SDL_RenderFillRect(_gRenderer, &rect);
#endif

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

void Debug::FindTileUnderMouse(flVec2<int>& _mousePos)
{
	//will be < 0 if mouse is outside tile world.
	int32_t tileRow = (_mousePos.y - m_verticlePixelBuffer) / m_tilesDimPixels; //x is columns, y is rows
	int32_t tileColumn = (_mousePos.x - m_horizontalPixelBuffer) / m_tilesDimPixels;
	if (	tileRow >= 0
		&&	tileRow < m_debugWorldDim 
		&&	tileColumn >= 0
		&&	tileColumn < m_debugWorldDim)
	{
		int32_t newIndex = static_cast<uint32_t>(tileRow)*m_debugWorldDim + static_cast<uint32_t>(tileColumn);
		if (newIndex != m_tileUnderMouseIndex)
		{
			m_tileUnderMouseIndex = newIndex;
		}
	}
	else if (m_tileUnderMouseIndex > 0)
	{
		m_tileUnderMouseIndex = -1;
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
	//enable mouse pos label
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
	//reset grid
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_R)))
	{
		for (uint32_t i = 0; i < m_debugWorldSize; i++)
		{
			m_debugWorld[i].m_energy = 0;
		}
		m_pointsToPropagate.clear();
	}
}

void Debug::MouseMovementInput(flVec2<int> _mousePos)
{
	if (m_showMousePos)
	{
		MaintainMousePosLabel(_mousePos);
	}

	int32_t oldIndex = m_tileUnderMouseIndex;
	FindTileUnderMouse(_mousePos);
	if (oldIndex != m_tileUnderMouseIndex)
	{
		//mouse is over different tile to the one on last frame
		if (m_setTilesActive)
		{
			flPoint& pointUnderMouse = m_debugWorld[m_tileUnderMouseIndex];
			if (pointUnderMouse.m_energy < 7) //ROYGBIV
				pointUnderMouse.m_energy++;
		}
		else if (m_setTilesUnactive)
		{
			flPoint& pointUnderMouse = m_debugWorld[m_tileUnderMouseIndex];
			if (pointUnderMouse.m_energy > 0)
				pointUnderMouse.m_energy--;
		}
	}
}

void Debug::MouseDownInput(eMouseButtonType const _buttonType, flVec2<int32_t> _mousePos)
{
	//set active
	if (_buttonType == eMouseButtonType::LeftClick)
	{
		m_setTilesActive = true;
		if (m_tileUnderMouseIndex >= 0)
		{
			flPoint& pointUnderMouse = m_debugWorld[m_tileUnderMouseIndex];
			if (pointUnderMouse.m_energy < 7) //ROYGBIV
				pointUnderMouse.m_energy++;
		}
	}
	//set unactive
	if (_buttonType == eMouseButtonType::RightClick)
	{
		m_setTilesUnactive = true;
		if (m_tileUnderMouseIndex >= 0)
		{
			flPoint& pointUnderMouse = m_debugWorld[m_tileUnderMouseIndex];
			if (pointUnderMouse.m_energy > 0)
				pointUnderMouse.m_energy--;
		}
	}
}

void Debug::MouseUpInput(eMouseButtonType const _buttonType, flVec2<int32_t> _mousePos)
{
	//set active
	if (_buttonType == eMouseButtonType::LeftClick)
	{
		m_setTilesActive = false;
	}
	//set unactive
	if (_buttonType == eMouseButtonType::RightClick)
	{
		m_setTilesUnactive = false;
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

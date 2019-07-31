#include <stdio.h>
#include <assert.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include "Debug.h"

#include "Interfaces/InputCoreIF.h"
#include "Interfaces/RenderCoreIF.h"

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void Debug::DefineHeldInput()
{
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		//@TODO: Decrement propagation step
		printf("Left Pressed Debug Mode\n");
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		//@TODO: Increment propagation step
		printf("Right Pressed Debug Mode\n");
	}
}

void Debug::KeyPressedInput(SDL_Scancode const& _key)
{
	if (_key == SDL_SCANCODE_T)
	{
		ToggleTessalationDisplay();
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
		ToggleTessalationDisplay();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
Debug::Debug(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCore)
	: InputDelegate(_inputCore),
	  m_renderCoreIF(_renderCoreIF),
	  m_debugEnabled(false),
	  m_displayTessalation(false),
	  m_propagationRate(2.0f),
	  m_propagationMode(PropagationMode::SimpleDirectional)
{
	uint32_t centerIndex = m_debugWorldSize % 2 ? m_debugWorldSize/2 + 1 : m_debugWorldSize/2;
	m_debugWorld[centerIndex].m_energy = 1;
	m_debugWorld[centerIndex].m_direction = flPoint::Direction::ALL;

	m_pointsToPropagate.push_back(centerIndex);
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
		m_renderCoreIF.CreateFadeLabel("Debug Toggle", "Debug Enabled", displayColour, displayRect, 1500);
	}
	else
	{
		m_renderCoreIF.CreateFadeLabel("Debug Toggle", "Debug Disabled", displayColour, displayRect, 1500);
	}

	return m_debugEnabled;
}

bool const Debug::DebugEnabled
(
) const
{
	return m_debugEnabled;
}

void Debug::ToggleTessalationDisplay
(
)
{
	m_displayTessalation = !m_displayTessalation;
}

bool const Debug::DisplayTessalation
(
) const
{
	return m_displayTessalation;
}

void Debug::CyclePropModeLeft
(
)
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

void Debug::CyclePropModeRight
(
)
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

void Debug::PropagateAdjacent
(
)
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

void Debug::UnpropagateAdjacent
(
)
{
	std::vector<uint32_t>::iterator iter = m_pointsToPropagate.begin();
	std::vector<uint32_t>::iterator end = m_pointsToPropagate.end();
	for (; iter != end;)
	{
		uint32_t index = *iter;
		int32_t& curEnergy = m_debugWorld[index].m_energy;
		curEnergy -= 1;
		assert(curEnergy >= 0);
		//if ()
		//@TODO - Need to figure out what it means to run the propagation in reverse
	}
}

void Debug::ResetWorldGrid
(
)
{
	uint32_t centerIndex = m_debugWorldDim % 2 ? m_debugWorldDim / 2 + 1 : m_debugWorldDim / 2;
	for (uint8_t i = 0; i < m_debugWorldDim*m_debugWorldDim; i++)
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

flPoint const * const Debug::GetDebugWorldDataRead
(
) const
{
	return m_debugWorld;
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

uint32_t Debug::GetIndexDown
(
	uint32_t _currentPointIndex
)
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

uint32_t Debug::GetIndexLeft
(
	uint32_t _currentPointIndex
)
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

uint32_t Debug::GetIndexRight
(
	uint32_t _currentPointIndex
)
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
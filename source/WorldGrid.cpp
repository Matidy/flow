#include "WorldGrid.h"

#include "Interfaces/InputCoreIF.h"

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void WorldGrid::DefineHeldInput()
{
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		printf("Left Pressed Game Mode\n");
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		printf("Right Pressed Game Mode\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
WorldGrid::WorldGrid(InputCoreIF& _inputCore)
	: InputDelegate(_inputCore),
	m_nullPoint(flPoint(1, flVec2(), false))
{
}

bool WorldGrid::GenerateWorld
(
)
{
	for (uint32_t i = 0; i < TOTAL_WORLD_SIZE; i++)
	{
		flPoint& p = m_worldGrid[i];
		p.m_energy = i % 2; //would want random chance, but failing current access to RNG lib, settle for every other
		p.m_velocityVector = flVec2(0, 0);
	}

	return true;
}

bool WorldGrid::UpdateStep
(
	uint32_t _timeStep
)
{
	for (uint32_t i = 0; i < TOTAL_WORLD_SIZE; i++)
	{
		flPoint& p = m_worldGrid[i];
		// todo: want to look at propagation pattern, including redundencies when propagating diagonally
		//		 adjacent tiles in the same step. Set up a basic GUI & debug mode for this.
		// 

	// attraction

	// repulsion

	}
	

	return true;
}

flPoint& WorldGrid::GetPointUp
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex < WORLD_X_SIZE)
	{
		//top row, no up to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex-WORLD_X_SIZE];
	}
}

flPoint& WorldGrid::GetPointDown
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex > TOTAL_WORLD_SIZE-1 - WORLD_X_SIZE)
	{
		//bottom row, no down to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex + WORLD_X_SIZE];
	}
}

flPoint& WorldGrid::GetPointLeft
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex % WORLD_X_SIZE == 0)
	{
		//left most row, no left to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex - 1];
	}

}

flPoint& WorldGrid::GetPointRight
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex % WORLD_X_SIZE == WORLD_X_SIZE-1)
	{
		//right most row, no right to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex + 1];
	}
}
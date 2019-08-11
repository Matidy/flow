#include "WorldGrid.h"

#include "Interfaces/RenderCoreIF.h"
#include "Interfaces/InputCoreIF.h"

#include <SDL_render.h>

///////////////////////////////////////////////////////////////////////////////////////////////
WorldGrid::WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF)
	: RenderDelegate(_renderCoreIF),
	  InputDelegate(_inputCoreIF),
	  m_nullPoint(flPoint(1, flVec2(), false)),
	  m_cullingViewport(flVec2(0u, 0u))
{
	//can catch bad alloc exception here to handle not having enough space on Heap at init.
	m_worldGrid = new flPoint[Globals::TOTAL_WORLD_SIZE]; //flPoint - 20 bytes * TOTAL_WORLD_SIZE - 16777216 = 335,544,320 (335 MB)
}

WorldGrid::~WorldGrid()
{
	delete[] m_worldGrid;
}

bool WorldGrid::GenerateWorld
(
)
{
	//basic checkboard energy setup, black -> white -> black ...
	for (uint32_t i = 0; i < Globals::TOTAL_WORLD_SIZE; i++)
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
	/*
	for (uint32_t i = 0; i < Globals::TOTAL_WORLD_SIZE; i++)
	{
		flPoint& p = m_worldGrid[i];
		// todo: want to look at propagation pattern, including redundencies when propagating diagonally
		//		 adjacent tiles in the same step. Set up a basic GUI & debug mode for this.
		// 

	// attraction

	// repulsion

	}
	*/
	//1. Using m_worldGrid, find every flPoint within m_cullingViewport's bounds.
	//2. 
	

	return true;
}

flPoint& WorldGrid::GetPointUp
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex < Globals::WORLD_X_SIZE)
	{
		//top row, no up to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex-Globals::WORLD_X_SIZE];
	}
}

flPoint& WorldGrid::GetPointDown
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex > Globals::TOTAL_WORLD_SIZE-1 - Globals::WORLD_X_SIZE)
	{
		//bottom row, no down to get
		flPoint& nullPoint = m_nullPoint;
		return nullPoint;
	}
	else
	{
		return m_worldGrid[_currentPointIndex + Globals::WORLD_X_SIZE];
	}
}

flPoint& WorldGrid::GetPointLeft
(
	uint32_t _currentPointIndex
)
{
	if (_currentPointIndex % Globals::WORLD_X_SIZE == 0)
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
	if (_currentPointIndex % Globals::WORLD_X_SIZE == Globals::WORLD_X_SIZE-1)
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

/////////////////////				    ///////////////////////////////////////////////////////
//////////////////   from RenderDelegate   ///////////////////////////////////////////////////
/////////////////////				    //////////////////////////
void WorldGrid::DelegateDraw(SDL_Renderer * const _gRenderer) const
{

}

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
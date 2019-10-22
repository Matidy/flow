#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>
#include <functional>
#include <map>

#include <SDL_render.h>
#include <SDL_timer.h>
#include "WorldGrid.h"

#include "Data/ValRGBA.h"

#include "Interfaces/RenderCoreIF.h"
#include "Interfaces/InputCoreIF.h"


///////////////////////////////////////////////////////////////////////////////////////////////
WorldGrid::WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF)
	: RenderDelegate(_renderCoreIF),
	  InputDelegate(_inputCoreIF),
	  m_nullEnergy(flEnergy(0xffff, PhysicsLib::Vector2D<flEnergy::MoveVectorType>(), false)),
	  m_mouseCursorDim(32u, 32u),
	  m_cullingViewport(flVec2<float>(0.f, 0.f))
{
	assert(Globals::TOTAL_WORLD_SIZE < 0xffffffff);
	//can catch bad alloc exception here to handle not having enough space on Heap at init.
	m_worldGrid.resize(Globals::TOTAL_WORLD_SIZE);
	m_worldEnergy.resize(Globals::TOTAL_WORLD_SIZE/m_energyToSpaceRatio);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	m_nullSpace.m_energyIndex = Globals::WALL_INDEX;

	flVec2<int> mousePos;
	m_inputCoreIF.GetMousePos(mousePos);
	UpdateClaimRectBounds(mousePos);
}

WorldGrid::~WorldGrid()
{
	//delete[] m_worldGrid;
}

bool WorldGrid::GenerateWorld()
{
	//reset space
	for (uint32_t i = 0; i <m_worldGrid.size(); i++)
	{
		m_worldGrid[i].m_energyIndex = Globals::NULL_INDEX;
	}
	//@consider - good candidate for multi-threading to enable rest of game to carry on functioning if this takes several seconds to compute
	//generate energy our world will contain
	uint32_t basePos = 0;
	for (uint32_t i = 0; i < m_worldEnergy.size(); i++)
	{
		uint32_t offset = std::rand() % m_energyToSpaceRatio; //every 8 space tiles contains 1 energy element

		flEnergy& curEnergy = m_worldEnergy[i];
		curEnergy.Nullify(); //reset

		uint32_t indexInWorld = basePos + offset;
		curEnergy.m_energy = 1;
		curEnergy.m_indexInWorld = indexInWorld;
		curEnergy.m_movementVector.m_startingPoint = Pos1DToPos2D(indexInWorld);

		//move direction
		flEnergy::MoveVectorType base = static_cast<flEnergy::MoveVectorType>(std::rand() % 2 ? 1 : -1); //-1 to +1
		flEnergy::MoveVectorType variable = static_cast<flEnergy::MoveVectorType>((std::rand() % 17) - 8); //-8 to +8
		variable += variable == 0 ? 1 : 0; //fudge to avoid 0 values

		flVec2<flEnergy::MoveVectorType>& direction = curEnergy.m_movementVector.m_direction; //direction is ratio of x to y moves
		if (std::rand() % 2)
		{
			direction.x = base;
			direction.y = variable;
		}
		else
		{
			direction.x = variable;
			direction.y = base;
		}

		//scalar stores how many cells to move in a second
		curEnergy.m_movementVector.m_scalar = static_cast<flEnergy::MoveVectorType>(std::rand() % 32 + 1);

		basePos += m_energyToSpaceRatio;
	}

#if 0
	//sort flEnergy array by move speed - fastest to slowest
	std::qsort(&m_worldEnergy[0], m_worldEnergy.size(), sizeof(flEnergy), [](const void *a, const void *b) -> int
	{
		float arg1 = static_cast<const flEnergy*>(a)->m_movementVector.m_scalar;
		float arg2 = static_cast<const flEnergy*>(b)->m_movementVector.m_scalar;

		if (arg1 > arg2) return -1;
		if (arg1 < arg2) return 1;
		return 0;
	});
#endif

	//set m_worldGrid array values to store the index of the flEnergy element contained in their space
	for (uint32_t i = 0; i < m_worldEnergy.size(); i++)
	{
		flEnergy const& curEnergy = m_worldEnergy[i];
		flSpace& associatedSpace = m_worldGrid[curEnergy.m_indexInWorld];
		associatedSpace.m_energyIndex = i;
	}

	/***************************************************
	**** Initial world generation ideas ****
	**************************************
	* 1. Generate islands in the world that are unmovable.
	*		i.   generate a number for the number of islands we'll have
	*		ii.  randomly generate a starting position for each island
	*			ei.  next level would be to generate a techtonic plate-esque subs division of world and to generate points along the edges of the sub-division
	*		iii. generate an orientation along as the
	*		iv.  generate a height for each island and, using RNG & exponetial function applied to height, calc how far island should expand in
	*			 each direction.
	* 2. Generate a number for the number of remaining tiles that should be active/contain energy and generate a list of indexes of that length
	*    from list of remaining empty tiles.
	* 3. For each point selected to contain energy, generate a random direction for it to starting moving and a random initial speed
	*/

	return true;
}

template<typename T>
void MoveTowardsZero(T& o_integer, T moveBy)
{
	if (o_integer == 0)
		return;

	if (moveBy >= abs(o_integer))
		o_integer = 0.f;
	else
	{
		int32_t moveDir = o_integer > 0 ? -1 : +1;
		o_integer += moveBy*static_cast<T>(moveDir);
	}
}

bool WorldGrid::UpdateStep
(
	uint32_t _timeStep
)
{
	//physics movement and collision processing
	{
		uint32_t frameStartTime = m_cumulativeFrameTime;
		m_cumulativeFrameTime += _timeStep;
		uint32_t frameEndTime = m_cumulativeFrameTime;
		for (uint32_t t = 1; t < _timeStep+1; t++)
		{
			for (uint32_t i = 0; i < m_worldEnergy.size(); i++)
			{
				flEnergy& energy = m_worldEnergy[i];
				uint32_t advanceEveryMs = static_cast<uint32_t>(1000.f/energy.m_movementVector.m_scalar);
				uint32_t startingAdvances = frameStartTime/advanceEveryMs;
				uint32_t endingAdvances = frameEndTime/advanceEveryMs;

				assert(startingAdvances <= endingAdvances);
				for (uint32_t a = startingAdvances; a < endingAdvances; a++)
				{
					uint32_t advanceTime = (a+1)*advanceEveryMs;
					if (advanceTime == frameStartTime + t)
					{
						uint32_t spaceToMoveIntoIndex = Globals::NULL_INDEX;
						flEnergy::Direction movDirection = flEnergy::Direction::ALL;
						flVec2<flEnergy::MoveVectorType>& movementStore = energy.m_currentMovementStore;
						if (movementStore == 0)
						{
							//refresh movement vector
							movementStore = energy.m_movementVector.m_direction;
						}

						if (movementStore.x != 0)
						{
							if (movementStore.x > 0)
							{
								//set up for move right
								spaceToMoveIntoIndex = GetIndexRight(energy.m_indexInWorld);
								movDirection = flEnergy::Direction::RIGHT;
							}
							else
							{
								//set up for move left
								spaceToMoveIntoIndex = GetIndexLeft(energy.m_indexInWorld);
								movDirection = flEnergy::Direction::LEFT;
							}

							MoveTowardsZero(movementStore.x, 1.f);
						}
						else if (movementStore.y != 0)
						{
							if (movementStore.y > 0)
							{
								//set up for move up
								spaceToMoveIntoIndex = GetIndexUp(energy.m_indexInWorld);
								movDirection = flEnergy::Direction::UP;
							}
							else
							{
								//set up for move down
								spaceToMoveIntoIndex = GetIndexDown(energy.m_indexInWorld);
								movDirection = flEnergy::Direction::DOWN;
							}

							MoveTowardsZero(movementStore.y, 1.f);
						}

						//process movement/collisions
						flSpace& currentSpace = GetSpaceAtIndex(energy.m_indexInWorld);//m_worldGrid[curEnergy.m_indexInWorld];
						flSpace& spaceToMoveInto = GetSpaceAtIndex(spaceToMoveIntoIndex);
						if (spaceToMoveInto.m_energyIndex == Globals::NULL_INDEX)
						{
							//empty, just move
							spaceToMoveInto.m_energyIndex = i;
							currentSpace.m_energyIndex = Globals::NULL_INDEX;

							energy.m_indexInWorld = spaceToMoveIntoIndex;
						}
						else
						{
							//collision
							if (spaceToMoveInto.m_energyIndex == Globals::WALL_INDEX)
							{
								//at world bound, just reflect
								if (movDirection == flEnergy::Direction::UP || movDirection == flEnergy::Direction::DOWN)
								{
									energy.m_movementVector.m_direction.y *= -1;
									energy.m_currentMovementStore.y *= -1;
								}
								else if (movDirection == flEnergy::Direction::LEFT || movDirection == flEnergy::Direction::RIGHT)
								{
									energy.m_movementVector.m_direction.x *= -1;
									energy.m_currentMovementStore.x *= -1;
								}
							}
							else
							{
								flEnergy& collidedWith = GetEnergyAtIndex(spaceToMoveIntoIndex);

								typedef flEnergy::MoveVectorType MoveVecType;
								PhysicsLib::Vector2D<MoveVecType>& curMovVec = energy.m_movementVector;
								PhysicsLib::Vector2D<MoveVecType>& collidedMovVec = collidedWith.m_movementVector;
								MoveVecType* curMovDir = nullptr;
								MoveVecType* curMovDirOrth = nullptr;
								MoveVecType* collidedMovDir = nullptr;
								MoveVecType* collidedMovDirOrth = nullptr;
								if (movDirection == flEnergy::Direction::UP || movDirection == flEnergy::Direction::DOWN)
								{
									curMovDir = &curMovVec.m_direction.y;
									curMovDirOrth = &curMovVec.m_direction.x;
									collidedMovDir = &collidedMovVec.m_direction.y;
									collidedMovDirOrth = &collidedMovVec.m_direction.x;
								}
								else if (movDirection == flEnergy::Direction::LEFT || movDirection == flEnergy::Direction::RIGHT)
								{
									curMovDir = &curMovVec.m_direction.x;
									curMovDirOrth = &curMovVec.m_direction.y;
									collidedMovDir = &collidedMovVec.m_direction.x;
									collidedMovDirOrth = &collidedMovVec.m_direction.y;
								}
								assert(curMovDir);
								assert(curMovDirOrth);
								assert(collidedMovDir);
								assert(collidedMovDirOrth);

								MoveVecType curOldMovScalar = curMovVec.m_scalar / (abs(*curMovDir) + abs(*curMovDirOrth)); //how many movement vec iterations we perform per second
								MoveVecType curSpeedInCollDir = curOldMovScalar * *curMovDir;
								MoveVecType collidedOldMovScalar = collidedMovVec.m_scalar / (abs(*collidedMovDir) + abs(*collidedMovDirOrth));
								MoveVecType collidedSpeedInCollDir = collidedOldMovScalar * *collidedMovDir;
								assert(curSpeedInCollDir > 0 || curSpeedInCollDir < 0);
								assert(collidedSpeedInCollDir > 0 || collidedSpeedInCollDir < 0);

								//swap movement speed of energies for collision axis and scale orth movement value to temporarily represent speed in orth direction, not just direction
								*curMovDir = collidedSpeedInCollDir;
								*curMovDirOrth = *curMovDirOrth * curOldMovScalar;
								*collidedMovDir = curSpeedInCollDir;
								*collidedMovDirOrth = *collidedMovDirOrth * collidedOldMovScalar;

								//update movement vector scalar to contain new total speed (tile movements per second)
								MoveVecType curNewSpeed = round(abs(*curMovDir) + abs(*curMovDirOrth));
								curMovVec.m_scalar = curNewSpeed != 0.f ? curNewSpeed : 1.f;
								MoveVecType collNewSpeed = round(abs(*collidedMovDir) + abs(*collidedMovDirOrth));
								collidedMovVec.m_scalar = collNewSpeed != 0.f ? collNewSpeed : 1.f;
								assert(curMovVec.m_scalar > 0 || curMovVec.m_scalar < 0);
								assert(collidedMovVec.m_scalar > 0 || collidedMovVec.m_scalar < 0);

								//pseudo-nomalise movement vector so that the smallest element is 1
								MoveVecType curMovSmallest = abs(*curMovDir) > abs(*curMovDirOrth) ? abs(*curMovDirOrth) : abs(*curMovDir);
								*curMovDir = round(*curMovDir/curMovSmallest);
								*curMovDirOrth = round(*curMovDirOrth/curMovSmallest);
								MoveVecType collMovSmallest = abs(*collidedMovDir) > abs(*collidedMovDirOrth) ? abs(*collidedMovDirOrth) : abs(*collidedMovDir);
								*collidedMovDir = round(*collidedMovDir/collMovSmallest);
								*collidedMovDirOrth = round(*collidedMovDirOrth/collMovSmallest);

								energy.m_currentMovementStore.Zero();
								collidedWith.m_currentMovementStore.Zero();
							}
						}
					}
				}
			}
		}


		/****************************************************************
		****  PREVIOUS PHYSICS IMPLEMENTATION APPROACHES  ****
		************************************************************
		*	1. Trails
		*		Method:
		*		- Iterate through energy movements one by one, and for each, compute entire required movement for passed frame time.
		*		- energy marks the tiles it passes through with the time within the frame it was in each tile
		*		- Other energies then check the tile they're moving into as to whether another tile was in it for this frame, within the same time bound as they should be in it
		*			- In this case, we have a collision
		*				- Simulate a collision at the overlap tile and clear the movement data beyond the collision tile for the tile that already pre-calculated its movement
		*				- Recalculate the collided with energy's new path after the collision and continue computing the collision path for the collider energy
		*		Issues:
		*		- Energies may pass over the same tile spaces but not actually collide, i.e. in the tiles at different times on the frame. In this case, we need to store multiple energy data associated
		*		  with the tile space, which adds a certain ambiguity as to how big the data struct for each single space tile needs to be.
		*			- Possible solution: If this is a rare occurence, then it should be fair inexpensive to have a pre-allocted array that provides storage for associating a space tile index to a group
		*			  of energy data.
				- May end up computing multiple collisions, only to have a collision occur nearer to the start of the movement trail, making all the former collision computations irrelevant
					- PS: Do one full movement trail calculation before processing any overlaps. Process overlaps from closest to starting tile for each energy.
		*	2. Pre-compute all movement times and sort
		*			Method:
		*			- For each energy, compute when during this frame they'll move and group these by advance time in a sorted map
		*			- Then simply iterate through sorted map and advance each group in order.
		*			Issues:
		*			- Pre computed movement time data is for simplest case of changes in speed. When a speed change does occur (i.e. due to a collision), we'll need to have some way of ignoring/removing 
		*			  old data from the advance times map and inserting the newly computed move time data based on the energy's post collision speed which could get costly.
		*/

		/******************************************
		****  PHYSICS TODO  ****
		******************************
		*	- Add handling to intersection finder for verticle lines and parrallel lines
		*	- Fix conversion from Polar coordinates to Cartesian for
		*	1. Create new GameData game frame/layer for visualising and playing around with math functions to act as a interactive workbook
		*	   for when trying to solve difficult math problems
		*/
	}

	//update cursor
	{
		flVec2<int> mousePos;
		m_inputCoreIF.GetMousePos(mousePos);
		UpdateClaimRectBounds(mousePos);
		ClaimTiles();
	}

	return true;
}

void WorldGrid::UpdateClaimRectBounds(flVec2<int> _mousePos)
{
	{ //update m_claimRect's bounds
		flVec2<float> claimRectPosTL = PixelPosInTileWorld(static_cast<float>(_mousePos.x) - (m_mouseCursorDim.x/2.f), //@fix - think the issue is with this function - putting cursor in top left tile produces a negative pos
														   static_cast<float>(_mousePos.y) - (m_mouseCursorDim.y/2.f));
		flVec2<float> mouseCursorDimTW = (m_mouseCursorDim/static_cast<float>(Globals::WINDOW_WIDTH)) * m_cullingViewport.m_xExtension*2.f;
		ClaimRect claimRect;
		claimRect.m_leftCol		= static_cast<int32_t>(floor(claimRectPosTL.x));
		claimRect.m_rightCol	= static_cast<int32_t>(floor(claimRect.m_leftCol + mouseCursorDimTW.x));
		claimRect.m_topRow		= static_cast<int32_t>(floor(claimRectPosTL.y));
		claimRect.m_bottomRow	= static_cast<int32_t>(floor(claimRect.m_topRow + mouseCursorDimTW.y));

		//adjustments to wrap cursor edges to world edge if any box edges are within the world
		if (claimRect.m_leftCol < 0 && claimRect.m_rightCol > 0)
			claimRect.m_leftCol = 0;
		if (claimRect.m_rightCol > Globals::WORLD_X_SIZE && claimRect.m_leftCol < Globals::WORLD_X_SIZE)
			claimRect.m_rightCol = Globals::WORLD_X_SIZE;
		if (claimRect.m_topRow < 0 && claimRect.m_bottomRow > 0)
			claimRect.m_topRow = 0;
		if (claimRect.m_bottomRow > Globals::WORLD_Y_SIZE && claimRect.m_topRow < Globals::WORLD_Y_SIZE)
			claimRect.m_bottomRow = Globals::WORLD_Y_SIZE;

		//adjustments to make sure atleast one box is always highlighted if the cursor is within world bounds - for when viewport is zoomed in to less than one tile size
		if (claimRect.m_leftCol == claimRect.m_rightCol)
			claimRect.m_rightCol++;
		if (claimRect.m_topRow == claimRect.m_bottomRow)
			claimRect.m_bottomRow++;

		m_claimRect = claimRect;
	}

	m_cursorInWorld = false;
	if (	m_claimRect.m_leftCol >= 0
		&&	m_claimRect.m_rightCol <= Globals::WORLD_X_SIZE
		&&	m_claimRect.m_topRow >= 0
		&&	m_claimRect.m_bottomRow <= Globals::WORLD_Y_SIZE)
	{
		m_cursorInWorld = true;
	}

	SDL_Point upperLeft = { -1, -1 };
	SDL_Point bottomLeft = { -1, -1 };
	SDL_Point bottomRight = { -1, -1 };
	SDL_Point upperRight = { -1, -1 };
	if (m_cursorInWorld)
	{
		flVec2<float> upperLeftPixPos = TilePosToPixelPos(m_claimRect.m_leftCol, m_claimRect.m_topRow);
		upperLeft = { static_cast<int>(floor(upperLeftPixPos.x)), static_cast<int>(floor(upperLeftPixPos.y)) };

		flVec2<float> bottomLeftPixPos = TilePosToPixelPos(m_claimRect.m_leftCol, m_claimRect.m_bottomRow);
		bottomLeft = { static_cast<int>(floor(bottomLeftPixPos.x)), static_cast<int>(floor(bottomLeftPixPos.y)) };

		flVec2<float> bottomRightPixPos = TilePosToPixelPos(m_claimRect.m_rightCol, m_claimRect.m_bottomRow);
		bottomRight = { static_cast<int>(floor(bottomRightPixPos.x)), static_cast<int>(floor(bottomRightPixPos.y)) };

		flVec2<float> topRightPixPos = TilePosToPixelPos(m_claimRect.m_rightCol, m_claimRect.m_topRow);
		upperRight = { static_cast<int>(floor(topRightPixPos.x)), static_cast<int>(floor(topRightPixPos.y)) };
	}

	m_cursorBox[0] = upperLeft;
	m_cursorBox[1] = bottomLeft;
	m_cursorBox[2] = bottomRight;
	m_cursorBox[3] = upperRight;
	m_cursorBox[4] = upperLeft; //wrap back around to starting point to complete box
}

void WorldGrid::ClaimTiles()
{
	if (m_cursorInWorld)
	{
		if (m_claimBitMap != eClaimState::NoOp1 && m_claimBitMap != eClaimState::NoOp2)
		{
			uint32_t transformTo;
			if (m_claimBitMap == eClaimState::Claim)
				transformTo = Globals::WALL_INDEX;
			else if (m_claimBitMap == eClaimState::Unclaim)
				transformTo = Globals::NULL_INDEX;

			for (int32_t row = m_claimRect.m_topRow; row < m_claimRect.m_bottomRow; row++)
			{
				for (int32_t col = m_claimRect.m_leftCol; col < m_claimRect.m_rightCol; col++)
				{
					uint32_t index = Pos2DToPos1D(flVec2<int>(col, row));
					flSpace& iSpace = m_worldGrid[index];
					if (iSpace.m_energyIndex >= Globals::WALL_INDEX) //don't change tiles containing energy
					{
						iSpace.m_energyIndex = transformTo;
					}
				}
			}
		}
	}
}

//brief - get distances in grid tiles from center to edge of worldGrid
flVec2<float> const WorldGrid::GetCenterToEdgeOffsets() const
{
	flVec2<float> widthHeightOffset;
	widthHeightOffset.x = static_cast<float>(Globals::WORLD_X_SIZE*Globals::TILE_DRAW_DIMENSIONS) / 2;
	widthHeightOffset.y = static_cast<float>(Globals::WORLD_Y_SIZE*Globals::TILE_DRAW_DIMENSIONS) / 2;

	return widthHeightOffset;
}


flVec2<int32_t> WorldGrid::Pos1DToPos2DInt(uint32_t _index)
{
	assert(_index < Globals::TOTAL_WORLD_SIZE);
	flVec2<int32_t> pos2D;
	pos2D.x = static_cast<int32_t>(_index % Globals::WORLD_X_SIZE);
	pos2D.y = static_cast<int32_t>(_index / Globals::WORLD_X_SIZE);
	assert(pos2D.x < Globals::WORLD_X_SIZE
		&& pos2D.y < Globals::WORLD_Y_SIZE);

	return pos2D;
}
flVec2<flEnergy::MoveVectorType> WorldGrid::Pos1DToPos2D(uint32_t _index)
{
	assert(_index < Globals::TOTAL_WORLD_SIZE);
	flVec2<flEnergy::MoveVectorType> pos2D;
	pos2D.x = static_cast<flEnergy::MoveVectorType>(_index % Globals::WORLD_X_SIZE);
	pos2D.y = static_cast<flEnergy::MoveVectorType>(_index / Globals::WORLD_X_SIZE);
	assert(static_cast<int32_t>(round(pos2D.x)) < Globals::WORLD_X_SIZE
		&& static_cast<int32_t>(round(pos2D.y)) < Globals::WORLD_Y_SIZE);

	return pos2D;
}

uint32_t WorldGrid::Pos2DToPos1D(flVec2<int32_t> _pos2D)
{
	assert(_pos2D.x < Globals::WORLD_X_SIZE
		&& _pos2D.y < Globals::WORLD_Y_SIZE);
	uint32_t index = _pos2D.y * Globals::WORLD_X_SIZE + _pos2D.x; //small values for y (<< 0) can still make a whole number when multiplied by WORLD_X_SIZE
	assert(index < Globals::TOTAL_WORLD_SIZE);

	return index;
}

flSpace& WorldGrid::GetSpaceAtIndex(uint32_t _index) 
{
	if (_index == Globals::NULL_INDEX)
	{
		return m_nullSpace;
	}
	else
	{
		return m_worldGrid[_index];
	}
}

flEnergy& WorldGrid::GetEnergyAtIndex(uint32_t _index) 
{
	if (_index == Globals::NULL_INDEX)
	{
		return m_nullEnergy;
	}
	else
	{
		uint32_t energyIndex = m_worldGrid[_index].m_energyIndex;
		return m_worldEnergy[energyIndex];
	}
}

uint32_t WorldGrid::GetIndexUp(uint32_t _currentPointIndex) const
{
	if (_currentPointIndex < Globals::WORLD_X_SIZE)
	{
		//top row, no up to get
		return Globals::NULL_INDEX;
	}
	else
	{
		return _currentPointIndex - Globals::WORLD_X_SIZE;
	}
}
uint32_t WorldGrid::GetIndexDown(uint32_t _currentPointIndex) const
{
	if (_currentPointIndex > Globals::TOTAL_WORLD_SIZE-1 - Globals::WORLD_X_SIZE)
	{
		//bottom row, no down to get
		return Globals::NULL_INDEX;
	}
	else
	{
		return _currentPointIndex + Globals::WORLD_X_SIZE;
	}
}
uint32_t WorldGrid::GetIndexLeft(uint32_t _currentPointIndex) const
{
	if (_currentPointIndex % Globals::WORLD_X_SIZE == 0)
	{
		//left most row, no left to get
		return Globals::NULL_INDEX;
	}
	else
	{
		return _currentPointIndex - 1;
	}
}
uint32_t WorldGrid::GetIndexRight(uint32_t _currentPointIndex) const
{
	if (_currentPointIndex % Globals::WORLD_X_SIZE == Globals::WORLD_X_SIZE-1)
	{
		return Globals::NULL_INDEX;
	}
	else
	{
		return _currentPointIndex + 1;
	}
}

/* brief  - get (row, col) of tile that contains given pixel pos
*  return - x - row
*		  - y - column
*/
flVec2<float> WorldGrid::PixelPosInTileWorld(uint32_t _pixelX, uint32_t _pixelY) const
{
	flVec2<float> tileWorldPos;
	//re-orient viewport coors to have 0,0 set at the top left corner of the tile world and to have m_pos represent the top left point of the viewport
	tileWorldPos.x = (-1*m_cullingViewport.m_pos.y) + (Globals::WORLD_Y_SIZE/2.f) - m_cullingViewport.m_yExtension;
	tileWorldPos.y = m_cullingViewport.m_pos.x + (Globals::WORLD_X_SIZE/2.f) - m_cullingViewport.m_xExtension; //x & y flipped here as using x to store rows, but rows corresponds to a measure in the Y axis.

	flVec2<float> offsetWithinViewport;
	float windowToViewportScalar = (m_cullingViewport.m_xExtension * 2.f) / static_cast<float>(Globals::WINDOW_WIDTH);
	offsetWithinViewport.x = static_cast<float>(_pixelY) * windowToViewportScalar;
	offsetWithinViewport.y = static_cast<float>(_pixelX) * windowToViewportScalar;

	tileWorldPos.x = tileWorldPos.x + offsetWithinViewport.x;
	tileWorldPos.y = tileWorldPos.y + offsetWithinViewport.y;

	return flVec2<float>(tileWorldPos.y, tileWorldPos.x);
}

flVec2<float> WorldGrid::TilePosToPixelPos(int32_t _col, int32_t _row) const
{
	flVec2<float> viewportTL;
	viewportTL.x = m_cullingViewport.m_pos.x + static_cast<float>(Globals::WORLD_X_SIZE)/2.f - m_cullingViewport.m_xExtension;
	viewportTL.y = (-1*m_cullingViewport.m_pos.y) + static_cast<float>(Globals::WORLD_Y_SIZE)/2.f - m_cullingViewport.m_yExtension;

	flVec2<float> tilePosVP;
	tilePosVP.x = static_cast<float>(_col) - viewportTL.x;
	tilePosVP.y = static_cast<float>(_row) - viewportTL.y;

	return tilePosVP * GetTileToPixelScalar();
}

/////////////////////				    ///////////////////////////////////////////////////////
//////////////////   from RenderDelegate   ///////////////////////////////////////////////////
/////////////////////				    //////////////////////////
uint32_t FloatToPixels(float const _inputDimension, float& _accRoundError)
{
	uint32_t outputPixelDim = static_cast<uint32_t>(round(_inputDimension));
	_accRoundError += static_cast<float>(outputPixelDim) - _inputDimension;

	//adjust pixel bounds if we've accumulated enough error for a whole pixel
	if (_accRoundError >= 1.f)
	{
		outputPixelDim -= 1;
		_accRoundError -= 1.f;
	}
	else if (_accRoundError <= -1.f)
	{
		outputPixelDim += 1;
		_accRoundError += 1.f;
	}

	return outputPixelDim;
}

//brief - Converts from float to int value representing pixel width, and accumulates error from rounding,
//		  modifying a future conversion by one pixel once we've accumulated one pixel worth of error.
uint32_t FloatToPixelsX(float const _inputDimension, bool const _zeroAccumulator = false)
{
	static float accRoundErrorX = 0.f; //accumulated error
	if (_zeroAccumulator)
		accRoundErrorX = 0.f;

	return FloatToPixels(_inputDimension, accRoundErrorX);
}

//brief - Converts from float to int value representing pixel height, and accumulates error from rounding,
//		  modifying a future conversion by one pixel once we've accumulated one pixel worth of error.
uint32_t FloatToPixelsY(float const _inputDimension, bool const _zeroAccumulator = false)
{
	static float accRoundErrorY = 0.f; //accumulated error
	if (_zeroAccumulator)
		accRoundErrorY = 0.f;

	return FloatToPixels(_inputDimension, accRoundErrorY);
}

void WorldGrid::DelegateDraw(SDL_Renderer * const _gRenderer) const
{
	//// Drawing world points within m_cullingViewport. Currently using squares to represent points through SDL's 2D rendering API
	//// WorldGrid tiles do not have dimensions within a space, only a position within a 2D array/grid. As such, m_cullingViewport's
	//// dimensions are defined in terms of points it covers. m_cullingViewport's dimensions are floats as it is possible for it to
	//// only partially contain a point row/column as it zooms and moves about the world points. Algorithm is below in parts:
	////	1. For each bound of the viewport (top, bottom, left, right), 
	////		i.  find the row/column that contains the bound.
	////		ii. calculate how much of that row/column is within the bound
	////	2. If the viewport bound is beyond the corresponding edge of the world, calculate the distance between the two
	////	3. Draw the points within the viewport bounds as squares
	////		i.  move the starting draw point to the point at the distance between the viewport edge and the world edge (as calculated in 2.)
	////		ii. for each point within the viewport (found in one), draw a square, such that the viewport fills the entire window.

	flVec2<float> const offsetsFromCenterToEdge = GetCenterToEdgeOffsets(); //number of points from center of world to edge

	////////////
	//// 1. @consider - could I replace this with just using the viewport bounda as the rows/cols at the edge of the viewport as viewport size is defined in terms of points? 
	////////
	//get row index for FIRST ROW INSIDE viewport and how much height off that row is not culled
	uint32_t topNonCulledRowIndex = 0; //start from top most row
	float topRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportTopY = m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension;
		float curRowBottomY = offsetsFromCenterToEdge.y - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with bottom edge of top row

		while (curRowBottomY >= viewportTopY) //move down through rows until we find one at least partly within cull boundary
		{
			//current row is totally outside culling boundary, move down one row
			topNonCulledRowIndex++;
			curRowBottomY -= static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);

			if (topNonCulledRowIndex == Globals::WORLD_Y_SIZE)
				break;
		}

		//current row is partly or fully within boundary, calculate how much is within boundary
		float curRowTopY = curRowBottomY + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportTopY < curRowTopY)
		{
			topRowHeightWithinBound -= abs(curRowTopY - viewportTopY);
		}
	}
	//get row index for LAST ROW INSIDE viewport, and how much height off that row is not culled
	int32_t bottomNonCulledRowIndex = Globals::WORLD_Y_SIZE-1; //start from bottom most row
	float bottomRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportBottomY = m_cullingViewport.m_pos.y - m_cullingViewport.m_yExtension;
		float curRowTopY = -offsetsFromCenterToEdge.y + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with top edge of bottom row

		while (curRowTopY <= viewportBottomY) //move up through rows until we find one at least partly within cull boundary
		{
			//current row is totally outside culling boundary, move up one row
			bottomNonCulledRowIndex--;
			curRowTopY += static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);

			if (bottomNonCulledRowIndex == -1)
				break;
		}

		//current row is partly or fully within boundary, calculate how much is within boundary
		float curRowBottomY = curRowTopY - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportBottomY > curRowBottomY)
		{
			bottomRowHeightWithinBound -= abs(curRowBottomY - viewportBottomY);
		}
	}
	//get column index for LEFT-MOST COLUMN INSIDE viewport, and how much width off that column is not culled
	uint32_t leftNonCulledColIndex = 0; //start from left-most column
	float leftColWidthWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of column is within bounds to start
	{
		float viewportLeftX = m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension;
		float curColRightX = -offsetsFromCenterToEdge.x + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with right edge of left-most column

		while (curColRightX <= viewportLeftX) //move right through columns until we find one at least partly within cull boundary
		{
			//current col is totally outside culling boundary, move right one col
			leftNonCulledColIndex++;
			curColRightX += static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);

			if (leftNonCulledColIndex == Globals::WORLD_X_SIZE)
				break;
		}

		//current col is partly or fully within boundary, calculate how much is within boundary
		float curColLeftX = curColRightX - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportLeftX > curColLeftX)
		{
			leftColWidthWithinBound -= abs(curColLeftX - viewportLeftX);
		}
	}
	//get column index for RIGHT-MOST COLUMN INSIDE viewport, and how much width off that column is not culled
	int32_t rightNonCulledColIndex = Globals::WORLD_X_SIZE-1; //start from right-most column
	float rightColWidthWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of column is within bounds to start
	{
		float viewportRightX = m_cullingViewport.m_pos.x + m_cullingViewport.m_xExtension;
		float curColLeftX = offsetsFromCenterToEdge.x - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with left edge of right-most column
		
		while (curColLeftX >= viewportRightX) //move left through columns until we find one at least partly within cull boundary
		{
			//current col is totally outside culling boundary, move left one col
			rightNonCulledColIndex--;
			curColLeftX -= static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);

			if (rightNonCulledColIndex == -1)
				break;
		}

		//current col is partly or fully within boundary, calculate how much is within boundary
		float curColRightX = curColLeftX + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportRightX < curColRightX)
		{
			rightColWidthWithinBound -= abs(curColRightX - viewportRightX);
		}
	}

	////////////
	//// 2.
	////////
	//if viewport totally contains world, get border between viewport bounds and edge of world (only need top and left as drawing starts from top-left of screen)
	//top
	float distTopViewportToWorld = 0.f;
	if (offsetsFromCenterToEdge.y < m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension)
		distTopViewportToWorld = (m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension) - offsetsFromCenterToEdge.y;
	//left
	float distLeftViewportToWorld = 0.f;
	if (-offsetsFromCenterToEdge.x > m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension)
		distLeftViewportToWorld = abs((m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension) + offsetsFromCenterToEdge.x); 

#define RENDER_TIMER 0
#if RENDER_TIMER
	//currently taking ~16ms so our whole frame time
	uint32_t startTime = SDL_GetTicks();
#endif

	////////////
	//// 3. @consider - to make this runs faster (profile as you go):
	////					1. GPU rendering
	////					2. Shift values that exist across multiple frame in back-buffer in accordance with viewport movement, and then pull data from WorldGrid to fill new part of WorldGrid
	////					   made visible by viewport movement.
	////////
	if (topNonCulledRowIndex < Globals::WORLD_Y_SIZE
		&& bottomNonCulledRowIndex >= 0
		&& leftNonCulledColIndex < Globals::WORLD_X_SIZE
		&& rightNonCulledColIndex >= 0) //check viewport actually contains some part of the world
	{
		////    INDEXING 1D array as 2D  //////////////////////////////////////////////////
		////  indexs are read from Top-Left to Bottom-Right
		////  general indexing rule  -  (row * WORLD_X_SIZE) + col
		////
		////				   index  ->  (row, col)		  
		////					   0  ->  (0, 0)													  
		////		1 * WORLD_X_SIZE  ->  (1, 0)
		////					   4  ->  (0, 4)
		////  17 * WORLD_X_SIZE + 84  ->  (17, 84)	-	indexing the element in the 18th row, 85th column
		////
		////  index / WORLD_X_SIZE	  ->	rowIndex (ignoring remainder/flooring output)
		////  index % WORLD_X_SIZE	  ->	columnIndex
		////
		///////////////////////////////////////////////////////////////
		////    SDL Rendering  //////////////////////////////////////////////////////////
		////  Render coordinates start at (0, 0) from the top left of the window.
		////  Window's pixel array starts at index (0, 0) and goes to (WINDOW_WIDTH-1, WINDOW_HEIGHT-1).
		////  Globals::WINDOW_WIDTH and WINDOOW_HEIGHT are passed into SDL_CreateWindow(...) so are literally the pixel
		////  dimensions of the Window that gets displayed on screen (i.e. same dimensions will take up a larger screen space
		////  on smaller monitors).
		////
		///////////////////////////////////////////////////////////////

		float viewportToWindowScalar = Globals::WINDOW_WIDTH / (m_cullingViewport.m_xExtension * 2);
		float pixelsPerTileDim = Globals::TILE_DRAW_DIMENSIONS * viewportToWindowScalar;

		SDL_Rect drawRect;
		float pixelsLeftWorldBorder = distLeftViewportToWorld * viewportToWindowScalar;
		float pixelsTopWorldBorder = distTopViewportToWorld * viewportToWindowScalar;
		drawRect.x = 0u + FloatToPixelsX(pixelsLeftWorldBorder, true);
		drawRect.y = 0u + FloatToPixelsY(pixelsTopWorldBorder, true);

		uint32_t currentRow = topNonCulledRowIndex;

		//iterate through m_worldGrid and draw tiles within index bound
		for (uint32_t index = topNonCulledRowIndex * Globals::WORLD_X_SIZE + leftNonCulledColIndex; //index starts at top-left element in non-culled part of grid
			index < bottomNonCulledRowIndex * Globals::WORLD_X_SIZE + (rightNonCulledColIndex + 1);  //index terminates at bottom-right element in non-culled part of grid
			)
		{
			if (index % Globals::WORLD_X_SIZE == leftNonCulledColIndex)
			{
				//first element in row - set pixel height for this row			
				float tilePixelHeight = pixelsPerTileDim;
				if (index / Globals::WORLD_X_SIZE == topNonCulledRowIndex)
					tilePixelHeight = topRowHeightWithinBound * viewportToWindowScalar;
				if (index / Globals::WORLD_X_SIZE == bottomNonCulledRowIndex)
					tilePixelHeight = bottomRowHeightWithinBound * viewportToWindowScalar;
				drawRect.h = FloatToPixelsY(tilePixelHeight);
			}

			//set pixel width for this tile
			float tilePixelWidth = pixelsPerTileDim;
			if (index % Globals::WORLD_X_SIZE == leftNonCulledColIndex)
				tilePixelWidth = leftColWidthWithinBound * viewportToWindowScalar;
			if (index % Globals::WORLD_X_SIZE == rightNonCulledColIndex)
				tilePixelWidth = rightColWidthWithinBound * viewportToWindowScalar;
			drawRect.w = FloatToPixelsX(tilePixelWidth);

			// blue for 0, orange for > 0
			uint32_t energyIndex = m_worldGrid[index].m_energyIndex;
			ValRGBA drawColour;
			drawColour.a = 255u;
			if (energyIndex < Globals::NULL_INDEX)
			{
				if (energyIndex < Globals::WALL_INDEX)
				{
					//orange
					drawColour.r = 255;
					drawColour.g = 190;
					drawColour.b = 87;
				}
				else
				{
					//wall index
					//grey
					drawColour.r = 92;
					drawColour.g = 92;
					drawColour.b = 92;
				}
			}
			else
			{
				//black
				drawColour.r = 0;
				drawColour.g = 0;
				drawColour.b = 0;
			}

			SDL_SetRenderDrawColor(_gRenderer, drawColour.r, drawColour.g, drawColour.b, drawColour.a);
			SDL_RenderFillRect(_gRenderer, &drawRect);

			//iterate to next
			drawRect.x += drawRect.w;
			index++;
			if (index > currentRow * Globals::WORLD_X_SIZE + rightNonCulledColIndex)
			{
				//at end of row, move down to first element of row below
				drawRect.x = 0u + FloatToPixelsX(pixelsLeftWorldBorder, true);
				drawRect.y += drawRect.h;

				currentRow++;
				index = currentRow * Globals::WORLD_X_SIZE + leftNonCulledColIndex;
			}
		}
	}

#if RENDER_TIMER
	uint32_t endTime = SDL_GetTicks();
	uint32_t timeDif = endTime - startTime;
	std::cout << "Start Time: " << startTime << std::endl;
	std::cout << "End Time: " << endTime << std::endl;
	std::cout << "Time Difference: " << timeDif << std::endl;
	std::cout << std::endl;
#endif

	//render mouse cursor box
	SDL_SetRenderDrawColor(_gRenderer, 255, 255, 255, 255);
	SDL_RenderDrawLines(_gRenderer, m_cursorBox, 5);
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void WorldGrid::DefineChordInput(uint32_t _timeStep) 
{
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_G)))
	{
		//regenerate world
		GenerateWorld();
	}
	if (m_inputCoreIF.TryKeyChord(KeyChordPair(SDL_SCANCODE_LCTRL, SDL_SCANCODE_R)))
	{
		//reset viewport pos/size
		m_cullingViewport.Reset();
	}

}

void WorldGrid::DefineHeldInput(uint32_t _timeStep)
{
	//viewport controls - pan
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_W) || m_inputCoreIF.IsPressed(SDL_SCANCODE_UP))
	{
		m_cullingViewport.m_pos.y += m_cullingViewport.m_xExtension * m_panSpeed * (0.001f*_timeStep); //only using xExtension to get consistent move speed in all directions
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_S) || m_inputCoreIF.IsPressed(SDL_SCANCODE_DOWN))
	{
		m_cullingViewport.m_pos.y -= m_cullingViewport.m_xExtension * m_panSpeed * (0.001f*_timeStep);
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_A) || m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		m_cullingViewport.m_pos.x -= m_cullingViewport.m_xExtension * m_panSpeed * (0.001f*_timeStep);
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_D) || m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		m_cullingViewport.m_pos.x += m_cullingViewport.m_xExtension * m_panSpeed * (0.001f*_timeStep);
	}
	//zoom in/out
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_Q))
	{
		m_cullingViewport.m_xExtension = m_cullingViewport.m_xExtension * (1.f + (m_zoomSpeed * (0.001f*_timeStep)));
		m_cullingViewport.m_yExtension = m_cullingViewport.m_yExtension * (1.f + (m_zoomSpeed * (0.001f*_timeStep)));
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_E))
	{
		m_cullingViewport.m_xExtension = m_cullingViewport.m_xExtension * (1.f - (m_zoomSpeed * (0.001f*_timeStep)));
		m_cullingViewport.m_yExtension = m_cullingViewport.m_yExtension * (1.f - (m_zoomSpeed * (0.001f*_timeStep)));
	}
}

void WorldGrid::KeyPressedInput(SDL_Scancode const& _key)
{
	//adjust cullingViewport pan speed
	if (_key == SDL_SCANCODE_Z)
	{
		m_panSpeed -= 0.5f;
		if (m_panSpeed < 0.f)
		{
			m_panSpeed = 0.f;
		}
	}
	if (_key == SDL_SCANCODE_X)
	{
		m_panSpeed += 0.5f;
	}

#ifdef _DEBUG
	//fixed increment debug panning
	if (_key == SDL_SCANCODE_Y)
	{
		m_cullingViewport.m_pos.y += m_cullingViewport.m_debugPanIncrement;
	}
	if (_key == SDL_SCANCODE_H)
	{
		m_cullingViewport.m_pos.y -= m_cullingViewport.m_debugPanIncrement;
	}
	if (_key == SDL_SCANCODE_G)
	{
		m_cullingViewport.m_pos.x -= m_cullingViewport.m_debugPanIncrement;
	}
	if (_key == SDL_SCANCODE_J)
	{
		m_cullingViewport.m_pos.x += m_cullingViewport.m_debugPanIncrement;
	}
	//adjust fixed increment
	if (_key == SDL_SCANCODE_T)
	{
		m_cullingViewport.m_debugPanIncrement -= 0.1f;
	}
	if (_key == SDL_SCANCODE_U)
	{
		m_cullingViewport.m_debugPanIncrement += 0.1f;
	}
#endif
}

void WorldGrid::MouseDownInput(eMouseButtonType const _buttonType, flVec2<int> _mousePos)
{
	if (_buttonType == eMouseButtonType::LeftClick)
	{
		m_claimBitMap |= eClaimState::Claim;
	}
	if (_buttonType == eMouseButtonType::RightClick)
	{
		m_claimBitMap |= eClaimState::Unclaim;
	}
}
void WorldGrid::MouseUpInput(eMouseButtonType const _buttonType, flVec2<int> _mousePos)
{
	if (_buttonType == eMouseButtonType::LeftClick)
	{
		m_claimBitMap &= ~eClaimState::Claim;
	}
	if (_buttonType == eMouseButtonType::RightClick)
	{
		m_claimBitMap &= ~eClaimState::Unclaim;
	}
}
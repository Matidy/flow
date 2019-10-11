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
	  m_cullingViewport(flVec2<float>(0.f, 0.f))
{
	assert(Globals::TOTAL_WORLD_SIZE < 0xffffffff);
	//can catch bad alloc exception here to handle not having enough space on Heap at init.
	m_worldGrid.resize(Globals::TOTAL_WORLD_SIZE);
	m_worldEnergy.resize(Globals::TOTAL_WORLD_SIZE/m_energyToSpaceRatio);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	m_nullSpace.m_energyIndex = Globals::WALL_INDEX;
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
								energy.m_movementVector.m_direction.y *= -1;
							else if (movDirection == flEnergy::Direction::LEFT || movDirection == flEnergy::Direction::RIGHT)
								energy.m_movementVector.m_direction.x *= -1;
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

							collidedWith.m_currentMovementStore.Zero();
						}

						energy.m_currentMovementStore.Zero();
					}
				}
			}
		}
	}

#if 0
	uint32_t frameStartTime = m_cumulativeFrameTime;
	m_cumulativeFrameTime += _timeStep;
	uint32_t frameEndTime = m_cumulativeFrameTime;
	if (frameEndTime - frameStartTime < 1000) //skip trying to process frames longer than one second as we're probably debugging
	{
		std::vector<CollisionIteration> enIndexesToCollCount; //energyIndexesToCollisionCounts

		/* Iterate through world Energy and calc any times within the previous frame duration that energy should have moved. Sort into buckets. */
		EnergyMovementBuckets movementBuckets;
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
				std::vector<CollisionIteration>& energyBucket = movementBuckets[advanceTime];
				energyBucket.push_back(CollisionIteration{ i, 0 });
			}

			enIndexesToCollCount.push_back(CollisionIteration{ i,0 });
		}

		/* movement buckets is groups of energy indexes, sorted by movement times
		*  iterate through and advance in order */
		for (std::pair<uint32_t, std::vector<CollisionIteration>> bucket : movementBuckets)
		{
			uint32_t currentIterationTimestep = bucket.first;
			std::vector<CollisionIteration>& collisionIterations = bucket.second;
			for (CollisionIteration collisionIteration : collisionIterations)
			{
				bool oldData = false;
				for (CollisionIteration const enIndexToCurCollIter : enIndexesToCollCount)
				{
					if (enIndexToCurCollIter.energyIndex == collisionIteration.energyIndex)
					{
						if (collisionIteration.iterationCount != enIndexToCurCollIter.iterationCount)
						{
							oldData = true;
						}
					}
				}

				if (!oldData)
				{
					flEnergy& curEnergy = m_worldEnergy[collisionIteration.energyIndex];
					uint32_t spaceToMoveIntoIndex = Globals::NULL_INDEX;
					flEnergy::Direction movDirection = flEnergy::Direction::ALL;
					flVec2<flEnergy::MoveVectorType>& movementStore = curEnergy.m_currentMovementStore;
					if (movementStore == 0)
					{
						//refresh movement vector
						movementStore = curEnergy.m_movementVector.m_direction;
					}

					if (movementStore.x != 0)
					{
						if (movementStore.x > 0)
						{
							//set up for move right
							spaceToMoveIntoIndex = GetIndexRight(curEnergy.m_indexInWorld);
							movDirection = flEnergy::Direction::RIGHT;
						}
						else
						{
							//set up for move left
							spaceToMoveIntoIndex = GetIndexLeft(curEnergy.m_indexInWorld);
							movDirection = flEnergy::Direction::LEFT;
						}

						MoveTowardsZero(movementStore.x, 1.f);
					}
					else if (movementStore.y != 0)
					{
						if (movementStore.y > 0)
						{
							//set up for move up
							spaceToMoveIntoIndex = GetIndexUp(curEnergy.m_indexInWorld);
							movDirection = flEnergy::Direction::UP;
						}
						else
						{
							//set up for move down
							spaceToMoveIntoIndex = GetIndexDown(curEnergy.m_indexInWorld);
							movDirection = flEnergy::Direction::DOWN;
						}

						MoveTowardsZero(movementStore.y, 1.f);
					}


					//process movement/collisions
					flSpace& currentSpace = GetSpaceAtIndex(curEnergy.m_indexInWorld);//m_worldGrid[curEnergy.m_indexInWorld];
					flSpace& spaceToMoveInto = GetSpaceAtIndex(spaceToMoveIntoIndex);
					if (spaceToMoveInto.m_energyIndex == Globals::NULL_INDEX)
					{
						//empty, just move
						spaceToMoveInto.m_energyIndex = collisionIteration.energyIndex;
						currentSpace.m_energyIndex = Globals::NULL_INDEX;

						curEnergy.m_indexInWorld = spaceToMoveIntoIndex;
					}
					else
					{
						//collision
						if (spaceToMoveInto.m_energyIndex == Globals::WALL_INDEX)
						{
							//at world bound, just reflect
							if (movDirection == flEnergy::Direction::UP || movDirection == flEnergy::Direction::DOWN)
								curEnergy.m_movementVector.m_direction.y *= -1;
							else if (movDirection == flEnergy::Direction::LEFT || movDirection == flEnergy::Direction::RIGHT)
								curEnergy.m_movementVector.m_direction.x *= -1;
						}
						else
						{
							flEnergy& collidedWith = m_worldEnergy[spaceToMoveInto.m_energyIndex];
							flEnergy::MoveVectorType* curMovDir = nullptr;
							flEnergy::MoveVectorType* curMovDirOrth = nullptr;
							flEnergy::MoveVectorType* collidedWithMovDir = nullptr;
							flEnergy::MoveVectorType* collidedWithMovDirOrth = nullptr;
							if (movDirection == flEnergy::Direction::UP || movDirection == flEnergy::Direction::DOWN)
							{
								curMovDir = &curEnergy.m_movementVector.m_direction.y;
								curMovDirOrth = &curEnergy.m_movementVector.m_direction.x;
								collidedWithMovDir = &collidedWith.m_movementVector.m_direction.y;
								collidedWithMovDirOrth = &collidedWith.m_movementVector.m_direction.x;
							}
							else if (movDirection == flEnergy::Direction::LEFT || movDirection == flEnergy::Direction::RIGHT)
							{
								curMovDir = &curEnergy.m_movementVector.m_direction.x;
								curMovDirOrth = &curEnergy.m_movementVector.m_direction.y;
								collidedWithMovDir = &collidedWith.m_movementVector.m_direction.x;
								collidedWithMovDirOrth = &collidedWith.m_movementVector.m_direction.y;
							}
							assert(curMovDir);
							assert(collidedWithMovDir);

							bool opposite = *curMovDir * *collidedWithMovDir > 0.f ? false : true; //whether energies were moving in opposite directions or not before the collision
							if (opposite)
							{
								//just reflect - no movement this frame
								*curMovDir *= -1;
								*collidedWithMovDir *= -1;
							}
#if 1
							else
							{
								//TODO - move direction != movement speed. Need to make sure (1, z) pattern of move vectors is preserved - @TODO - debug getting zero values for move directions/move speeds
								PhysicsLib::Vector2D<flEnergy::MoveVectorType>& curMovVec = curEnergy.m_movementVector;
								flEnergy::MoveVectorType curDirectionTotal = abs(curMovVec.m_direction.x) + abs(curMovVec.m_direction.y);
								flEnergy::MoveVectorType curSpeedInDir = (curMovVec.m_scalar/curDirectionTotal) * *curMovDir;
								flEnergy::MoveVectorType curSpeedInOrth = (curMovVec.m_scalar/curDirectionTotal) * *curMovDirOrth;
								assert(curSpeedInDir > 0 || curSpeedInDir < 0);
								assert(curSpeedInOrth > 0 || curSpeedInOrth < 0);

								PhysicsLib::Vector2D<flEnergy::MoveVectorType>& collidedMovVec = collidedWith.m_movementVector;
								flEnergy::MoveVectorType collidedDirectionTotal = abs(collidedMovVec.m_direction.x) + abs(collidedMovVec.m_direction.y);
								flEnergy::MoveVectorType collidedSpeedInDir = (collidedMovVec.m_scalar/collidedDirectionTotal) * *collidedWithMovDir;
								flEnergy::MoveVectorType collidedSpeedInOrth = (collidedMovVec.m_scalar/collidedDirectionTotal) * *collidedWithMovDirOrth;
								assert(collidedSpeedInDir > 0 || collidedSpeedInDir < 0);
								assert(collidedSpeedInOrth > 0 || collidedSpeedInOrth < 0);

								//swap speed in collision dir into vec move speeds
								curMovVec.m_scalar = abs(curSpeedInOrth) + abs(collidedSpeedInDir);
								collidedMovVec.m_scalar = abs(collidedSpeedInOrth) + abs(curSpeedInDir);

								//swap speeds in collision direction & swap speed values into orth movement vector elements to be normalised in next step
								*curMovDir = collidedSpeedInDir;
								*curMovDirOrth = curSpeedInOrth;
								*collidedWithMovDir = curSpeedInDir;
								*collidedWithMovDirOrth = collidedSpeedInOrth;

								//pseudo-normalise vectors, so that smallest element is 1 & swap mov vec elements in collision direction
								flEnergy::MoveVectorType curSmallestMovElement = *curMovDir > *curMovDirOrth ? *curMovDirOrth : *curMovDir;
								*curMovDir = *curMovDir/curSmallestMovElement;
								*curMovDirOrth = *curMovDirOrth/curSmallestMovElement;

								flEnergy::MoveVectorType collidedSmallestMovElement = *collidedWithMovDir > *collidedWithMovDirOrth ? *collidedWithMovDirOrth : *collidedWithMovDir;
								*collidedWithMovDir = *collidedWithMovDir/collidedSmallestMovElement;
								*collidedWithMovDirOrth = *collidedWithMovDirOrth/collidedSmallestMovElement;

								

								//update iteration count so old movement times are ignored (as they were calculated from our previous speed)
								//caculate and add new movement times to movement bucket array
								uint32_t collisionIterCountForCur;
								uint32_t collisionIterCountForCollided;
								for (CollisionIteration& indexToCollCount : enIndexesToCollCount)
								{
									if (indexToCollCount.energyIndex == GetSpaceAtIndex(curEnergy.m_indexInWorld).m_energyIndex)
									{
										collisionIterCountForCur = ++indexToCollCount.iterationCount;
									}
									if (indexToCollCount.energyIndex == GetSpaceAtIndex(collidedWith.m_indexInWorld).m_energyIndex)
									{
										collisionIterCountForCollided = ++indexToCollCount.iterationCount;
									}
								}

								auto CalcNewMovementTimes = [&movementBuckets, currentIterationTimestep, frameEndTime]
								(PhysicsLib::Vector2D<flEnergy::MoveVectorType> const& _movementVector, uint32_t _energyIndex, uint32_t _collisionIteration) -> void
								{
									uint32_t advanceEveryMs = static_cast<uint32_t>(1000.f/_movementVector.m_scalar);
									uint32_t startingAdvances = currentIterationTimestep/advanceEveryMs;
									uint32_t endingAdvances = frameEndTime/advanceEveryMs;

									assert(startingAdvances <= endingAdvances);
									for (uint32_t a = startingAdvances; a < endingAdvances; a++)
									{
										uint32_t advanceTime = (a+1)*advanceEveryMs;
										std::vector<CollisionIteration>& energyBucket = movementBuckets[advanceTime];
										energyBucket.push_back(CollisionIteration{ _energyIndex, _collisionIteration });
									}
								};
								CalcNewMovementTimes(curMovVec, GetSpaceAtIndex(curEnergy.m_indexInWorld).m_energyIndex, collisionIterCountForCur);
								CalcNewMovementTimes(collidedMovVec, GetSpaceAtIndex(collidedWith.m_indexInWorld).m_energyIndex, collisionIterCountForCollided);
								
							}
#endif
							collidedWith.m_currentMovementStore.Zero();
						}

						curEnergy.m_currentMovementStore.Zero();
					}
				}
			}
		}
	}
#endif

#if 0
	//update movement, check & process collisions
	{
		/*
		*  1. Iterate through energy array one element at a time.
		*  2. 
		*
		*
		*/
		std::function<void(uint32_t, uint32_t)> ProcessMovementRecursive;
		ProcessMovementRecursive = [this, &ProcessMovementRecursive, _timeStep](uint32_t _waitingIndex, uint32_t _msAdvanced) -> void
		{
			flEnergy::MoveVectorType prevSpeed;
			for (uint32_t i = 0; i < _waitingIndex; i++)
			{
				flEnergy& curEnergy = m_worldEnergy[i];
				PhysicsLib::Vector2D<flEnergy::MoveVectorType>& curMovVec = curEnergy.m_movementVector;
				if (i == 0)
					prevSpeed = curMovVec.m_scalar; //base value

				uint32_t moveEveryMs = static_cast<uint32_t>(1000.f/curMovVec.m_scalar);
				if (moveEveryMs <= _timeStep + curEnergy.m_accumulatedMoveTime) //check if enough time has passed to move
				{

					/*@current - trying to figure out if accumulated movetime could affect order in which cells should move
					*		   - what is accumulated time/ms, per element, in this context?
					*/


					flEnergy::MoveVectorType speedDif = curMovVec.m_scalar - prevSpeed;
					if (speedDif > 0.f)
					{
						uint32_t msAdvanced = static_cast<uint32_t>(1000.f/speedDif);
						ProcessMovementRecursive(i, speedDif); //recursively iterate from start of array to update all elements that will have moved in the milliseconds we've advanced
					}

					//once we've returned from recursive calls, we're now free to process this index's movement
					//if ()
				}
				else
				{
					curEnergy.m_accumulatedMoveTime += _timeStep;
				}
			}
		};
		ProcessMovementRecursive(m_worldEnergy.size(), 0.f);
	}
#endif


//half finished code for points leaving movement trails in order to check if two points cross movement paths
#if 0
	for (uint32_t i = 0; i < Globals::TOTAL_WORLD_SIZE; i++)
	{
		//check for collisions and update pos
		{
			flPoint& startingPoint = m_worldGrid[i];
			if (startingPoint.m_energy > 0)
			{
				uint32_t startingIndex = i;
				startingPoint.m_timeEntered = 0;
				PhysicsLib::Vector2D<flPoint::MoveVectorType>& movVec = startingPoint.m_movementVector;
				//movVec.m_startingPoint; //currentPos in worldGrid (needed?)

				movVec.m_direction; //movement ratio
				movVec.m_scalar; //cells/tiles moved per second
				if (movVec.m_scalar > 0) //check if we're moving
				{
					uint32_t currentIndex = i;
					uint32_t moveEveryMS = 1000u/movVec.m_scalar; //how many milli-seconds between each move
					uint32_t moveIterationsToDo = _timeStep/moveEveryMS;
					uint32_t moveIterationsDone = 0;

					startingPoint.m_accumulatedMoveTime += _timeStep % moveEveryMS;
					if (startingPoint.m_accumulatedMoveTime >= moveEveryMS) //handle carried over time making up a whole movement
					{
						startingPoint.m_accumulatedMoveTime -= moveEveryMS;
						moveIterationsToDo++;
					}

					while (moveIterationsDone < moveIterationsToDo)
					{
						flPoint& currentPoint = m_worldGrid[currentIndex];

						//maintain a vector store checking how much of current move cycle ratio we've completed
						flVec2<flPoint::MoveVectorType>& movRatioStore = currentPoint.m_currentMovementStore;
						if (movRatioStore == 0)
						{
							movRatioStore = movVec.m_direction; //current move cycle exhausted - refresh
						}

						//get the point we're looking to move into next
						uint32_t nextIndex = Globals::TOTAL_WORLD_SIZE;
						if (movRatioStore.x != 0)
						{
							if (movRatioStore.x > 0)
							{
								movRatioStore.x--;
								nextIndex = GetIndexRight(currentIndex);
							}
							else if (movRatioStore.x < 0)
							{
								movRatioStore.x++;
								nextIndex = GetIndexLeft(currentIndex);
							}
						}
						else if (movRatioStore.y != 0)
						{
							if (movRatioStore.y > 0)
							{
								movRatioStore.y--;
								nextIndex = GetIndexUp(currentIndex);
							}
							else if (movRatioStore.y < 0)
							{
								movRatioStore.y++;
								nextIndex = GetIndexDown(currentIndex);
							}
						}
						assert(nextIndex < Globals::TOTAL_WORLD_SIZE);

						//process how we should handle trying to move into our next point
						flPoint& nextPoint = GetPoint(nextIndex);
						if (nextPoint.m_energy == 2)
						{
							if (currentPoint.m_timeEntered >= nextPoint.m_timeEntered)
							{
								if (currentPoint.m_timeExited <= nextPoint.m_timeExited)
								{

								}
							}
						}
						else if (nextPoint.m_energy == 1)
						{
							//end of movement this frame vs hasn't been processed yet
							//A: just calculate time bounds for this point to check against

							//just need to store for what bit of current frame time point was in each cell during its move to compare against

						}
						else if (nextPoint.m_energy == 0)
						{
							//empty space, no extra processing
							if (currentIndex != startingIndex) //using starting point as store of this point's values, will update it at end of move
							{
								currentPoint.m_energy = 2;
							}
							uint32_t timeBoundary = (moveIterationsDone+1) * moveEveryMS;
							currentPoint.m_timeExited = timeBoundary;
							nextPoint.m_timeEntered = timeBoundary;

							currentIndex = nextIndex;
						}

						//could return different types of null value for 'nullTop/nullBottom and nullLeft/nullRight' to inform which axis we 
						//should reflect
						moveIterationsDone++;
					}

					//referencing data stored in point's initial position throughout movement projection, so don't move data out of starting
					//point until we've finished iterating


				} //if scalar > 0
			}
		}
	}
#endif

//half finsihed code moving points and reflecting at world boundary
#if 0
				typedef flPoint::MoveVectorType flPointVecType;
				flVec2<flPointVecType>& currentPos = p.m_movementVector.m_startingPoint;
				flVec2<flPointVecType>& currentMoveVec = p.m_movementVector.m_direction;
				flVec2<flPointVecType> movThisFrame = currentMoveVec * 0.001f * static_cast<float>(_timeStep);
				flVec2<flPointVecType> nextPos = currentPos + movThisFrame; //projection with no collisions


				flVec2<int32_t> currentPosInt = flVec2<int32_t>(static_cast<int32_t>(round(currentPos.x)),
					static_cast<int32_t>(round(currentPos.y)));
				flVec2<int32_t> nextPosInt = flVec2<int32_t>(static_cast<int32_t>(round(nextPos.x)),
					static_cast<int32_t>(round(nextPos.y)));
				if (nextPosInt != currentPosInt) //only update pos if we've moved over at least one rounding boundary, e.g. 2.4 -> 2, 2.6 -> 3
				{
					/*** attraction ***/
					//@TODO


					/*** repulsion ***/
					/* Check collision between each particle. Axis-aligned reflections, RNG based off of movement vectors to decide if collisions
					*  should be veticle or horizontal reflection.
					*/
					// @TODO - After each collision will need to check collisions again for new reflected vector
					//		 - Thinking should only implement one level of collision check, no attraction and then the ability to push the points
					//		   around to give a basic gas particle simulation

					/* Check if either x or y are beyond world bounds. If they are, we just want to do a standard reflection along the edges of the world:
					*  if x bound then its the x coor movement that's brought us beyond the world bounds - minus x dist between starting point and
					*  world bound from x and then invert the remainder to get the reflected x pos. Similar thing for y. */
					{
						flPointVecType distToXBound, distToYBound;
						bool exitOut = false;
						if (nextPosInt.x < 0)
						{
							distToXBound = -0.5f - currentPos.x;
							float reflectedXMov = -1.f * (movThisFrame.x - distToXBound);
							nextPos.x = -0.5f + reflectedXMov;

							currentMoveVec.x = -currentMoveVec.x; //reverse movement vector x
							exitOut = true;
						}
						else if (nextPosInt.x > Globals::WORLD_X_SIZE-1)
						{
							distToXBound = static_cast<float>(Globals::WORLD_X_SIZE) - 0.4999f - currentPos.x;
							float reflectedXMov = -1 * (movThisFrame.x - distToXBound);
							nextPos.x = static_cast<float>(Globals::WORLD_X_SIZE) - 0.4999f + reflectedXMov;

							currentMoveVec.x = -currentMoveVec.x; //reverse movement vector x
							exitOut = true;
						}
						if (nextPos.y < 0)
						{
							distToYBound = -0.5f - currentPos.y;
							float reflectedYMov = -1 * (movThisFrame.y - distToYBound);
							nextPos.y = -0.5f + reflectedYMov;

							currentMoveVec.y = -currentMoveVec.y; //reverse movement vector y
							exitOut = true;
						}
						else if (nextPos.y > Globals::WORLD_Y_SIZE-1)
						{
							distToYBound = static_cast<float>(Globals::WORLD_Y_SIZE) - 0.4999f - currentPos.y;
							float reflectedYMov = -1 * (movThisFrame.y - distToYBound);
							nextPos.y = static_cast<float>(Globals::WORLD_Y_SIZE) -0.4999f + reflectedYMov;

							currentMoveVec.y = -currentMoveVec.y; //reverse movement vector y
							exitOut = true;
						}
						if (exitOut)
							continue; //exit out if reflected at world bounds - we'll update its pos on the next frame
					}
#if 0
					if (	 /*nextPosInt.x < 0 || */nextPosInt.x >= Globals::WORLD_X_SIZE
						||   nextPosInt.y < 0 || nextPosInt.y >= Globals::WORLD_Y_SIZE )
					{
						continue;
					}
#endif
					assert( nextPosInt.x >= 0 && nextPosInt.x < Globals::WORLD_X_SIZE
						 && nextPosInt.y >= 0 && nextPosInt.y < Globals::WORLD_Y_SIZE );
					
					uint32_t currentIndex = Pos2DToPos1D(currentPosInt);
					uint32_t nextIndex = Pos2DToPos1D(nextPosInt);
					assert(currentIndex != nextIndex);

					//move energy into its new worldGrid tile
					flPoint& currentPoint = m_worldGrid[currentIndex];
					flPoint& nextPoint = m_worldGrid[nextIndex];
					nextPoint = currentPoint;
					currentPoint.Nullify();

					nextPoint.m_movementVector.m_startingPoint = nextPos;
				}
				else
				{
					//update float store of current position
					currentPos = nextPos;
				}
#endif

	/******************************************
	****  PHYSICS TODO  ****
	******************************
	*	- Add handling to intersection finder for verticle lines and parrallel lines
	*	- Fix conversion from Polar coordinates to Cartesian for 
	*	1. Create new GameData game frame/layer for visualising and playing around with math functions to act as a interactive workbook
	*	   for when trying to solve difficult math problems
	*/

	return true;
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
				//orange
				drawColour.r = 255;
				drawColour.g = 190;
				drawColour.b = 87;
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
#include <cstdlib>
#include <cmath>
#include <iostream>

#include <SDL_render.h>
#include "WorldGrid.h"

#include "Data/ValRGBA.h"

#include "Interfaces/RenderCoreIF.h"
#include "Interfaces/InputCoreIF.h"



///////////////////////////////////////////////////////////////////////////////////////////////
WorldGrid::WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF)
	: RenderDelegate(_renderCoreIF),
	  InputDelegate(_inputCoreIF),
	  m_nullPoint(flPoint(1, flVec2<uint32_t>(), false)),
	  m_cullingViewport(flVec2<float>(0.f, 0.f))
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
		//// generation methods //////////////////////
		/*p.m_energy = (i + ((i/Globals::WORLD_X_SIZE)%2)) % 2;*/ //1 dim checkerboard

		p.m_energy = rand() % 2; //random
		////////////////////////////////
		p.m_velocityVector = flVec2<uint32_t>(0, 0);
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

	return true;
}

//breif - 
flVec2<float> const WorldGrid::GetCenterToEdgeOffsets() const
{
	flVec2<float> widthHeightOffset;
	widthHeightOffset.x = static_cast<float>(Globals::WORLD_X_SIZE*Globals::TILE_DRAW_DIMENSIONS) / 2;
	widthHeightOffset.y = static_cast<float>(Globals::WORLD_Y_SIZE*Globals::TILE_DRAW_DIMENSIONS) / 2;

	return widthHeightOffset;
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
//		  modifying a future conversion by a pixel once we've accumulated one pixel worth of error.
uint32_t FloatToPixelsX(float const _inputDimension, bool const _zeroAccumulator = false)
{
	static float accRoundErrorX = 0.f; //accumulated error
	if (_zeroAccumulator)
		accRoundErrorX = 0.f;

	return FloatToPixels(_inputDimension, accRoundErrorX);
}

//brief - Converts from float to int value representing pixel height, and accumulates error from rounding,
//		  modifying a future conversion by a pixel once we've accumulated one pixel worth of error.
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
	//// 1.
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

	////////////
	//// 3.
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
			flPoint const& pointAtIndex = m_worldGrid[index];
			ValRGBA drawColour;
			drawColour.a = 255u;
			if (pointAtIndex.m_energy > 0u)
			{
				//orange
				drawColour.r = 255;
				drawColour.g = 190;
				drawColour.b = 87;
			}
			else
			{
				//blue
				drawColour.r = 102;
				drawColour.g = 161;
				drawColour.b = 255;
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
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
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
	if (_key == SDL_SCANCODE_R)
	{
		m_cullingViewport.Reset();
	}

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
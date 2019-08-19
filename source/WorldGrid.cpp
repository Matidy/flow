#include <cstdlib>
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
	  m_cullingViewport(flVec2<float>(0u, 0u))
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
void WorldGrid::DelegateDraw(SDL_Renderer * const _gRenderer) const
{
	//1. Using m_worldGrid, find every flPoint within m_cullingViewport's bounds.
	//2. For each sqaure, create a draw square
	//		i. for sqaures at edge of viewport, set edge points to be equal to culling bounds.

	flVec2<float> const offsetsFromCenterToEdge = GetCenterToEdgeOffsets();
	//- trying to figure out how to do coordintes and position when comparing m_worldGrid and m_cullingViewport. Is looking at
	//  difference between worldDimensions and pixelDimensions. Thinking shouldn't consider tiles as pixels such that each can be
	//  broken down into a sub group of pixels for rendering and have the pixel representation scaled as desired.
	//- screen in only thing that has concept of coordinates. World has local position relative to its tile neighbours but doesn't
	//  need to have a concept of coordinates in any way.
	//- if worldGrid tiles don't have a concept of space, how can we say which ones are within a boundary?
	//		- impose tile/point concept of space at rendering stage
	//			1. tiles have a fixed size.
	//			2. center of world is always the point at half the world width and half the world height.
	
	//Then - cull all tiles with no part within culling viewport, go top, bottom, left, right as due to 1D layout can slice
	//		 beginning and end of array to do top and bottom cull without having to stitch non-contiguous blocks to get culled
	//		 world array.
	//     - float option for flVec2 as should really be using floats when regularly dividing values.



	//get row index for FIRST ROW INSIDE viewport and how much height off that row is not culled
	uint32_t topNonCulledRowIndex = 0; //start from top most row
	float topRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportTopY = m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension;
		float curRowBottomY = offsetsFromCenterToEdge.y - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with bottom edge of top row

		//move down through rows until we find one at least partly within cull boundary
		while (curRowBottomY > viewportTopY)
		{
			//current row is totally outside culling boundary, move down one row
			topNonCulledRowIndex++;
			curRowBottomY -= static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		}

		//current row is partly or fully within boundary, calculate how much is within boundary
		float curRowTopY = curRowBottomY + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportTopY < curRowTopY)
		{
			topRowHeightWithinBound -= abs(curRowTopY - viewportTopY);
		}
	}
	//get row index for LAST ROW INSIDE viewport, and how much height off that row is not culled
	uint32_t bottomNonCulledRowIndex = Globals::WORLD_Y_SIZE-1; //start from bottom most row
	float bottomRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportBottomY = m_cullingViewport.m_pos.y - m_cullingViewport.m_yExtension;
		float curRowTopY = -offsetsFromCenterToEdge.y + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with top edge of bottom row

		//move up through rows until we find one at least partly within cull boundary
		while (curRowTopY < viewportBottomY)
		{
			//current row is totally outside culling boundary, move up one row
			bottomNonCulledRowIndex--;
			curRowTopY += static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
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

		//move right through columns until we find one at least partly within cull boundary
		while (curColRightX < viewportLeftX)
		{
			//current col is totally outside culling boundary, move right one col
			leftNonCulledColIndex++;
			curColRightX += static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		}

		//current col is partly or fully within boundary, calculate how much is within boundary
		float curColLeftX = curColRightX - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportLeftX > curColLeftX)
		{
			leftColWidthWithinBound -= abs(curColLeftX - viewportLeftX);
		}
	}
	//get column index for RIGHT-MOST COLUMN INSIDE viewport, and how much width off that column is not culled
	uint32_t rightNonCulledColIndex = Globals::WORLD_Y_SIZE-1; //start from right-most column
	float rightColWidthWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of column is within bounds to start
	{
		float viewportRightX = m_cullingViewport.m_pos.x + m_cullingViewport.m_xExtension;
		float curColLeftX = offsetsFromCenterToEdge.x - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with left edge of right-most column

		//move left through columns until we find one at least partly within cull boundary
		while (curColLeftX > viewportRightX)
		{
			//current col is totally outside culling boundary, move left one col
			rightNonCulledColIndex--;
			curColLeftX -= static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		}

		//current col is partly or fully within boundary, calculate how much is within boundary
		float curColRightX = curColLeftX + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS);
		if (viewportRightX < curColRightX)
		{
			rightColWidthWithinBound -= abs(curColRightX - viewportRightX);
		}
	}

	//@consider - probably shouldn't use float values when doing culling operations as will then have to convert from float to int values when scaling culled worldGrid to
	//			  our window/screen's pixel array. Using floats obfuscates whole process and makes rounding errors/skipped pixels possible.
	uint32_t nonCulledWorldWidth = (rightNonCulledColIndex-1) - (leftNonCulledColIndex+1);	//width in tile cols
	//given tiles are square, calc how many pixels high/wide we should draw each tile to fill the screen
	uint32_t pixelsPerTileDim = ( Globals::WINDOW_WIDTH - static_cast<uint32_t>(leftColWidthWithinBound) - static_cast<uint32_t>(rightColWidthWithinBound) )
								/ nonCulledWorldWidth; 

	////  INDEXING 1D array as 2D  //////////////////////////////////////////////////
	////indexs are read from Top-Left to Bottom-Right
	////general indexing rule  -  (row * WORLD_X_SIZE) + col
	////
	////				   index  ->  (row, col)		  
	////					   0  ->  (0, 0)													  
	////		1 * WORLD_X_SIZE  ->  (1, 0)
	////					   4  ->  (0, 4)
	////17 * WORLD_X_SIZE + 84  ->  (17, 84)	-	indexing the element in the 18th row, 85th column
	////
	////index / WORLD_X_SIZE	  ->	rowIndex (ignoring remainder/flooring output)
	////index % WORLD_X_SIZE	  ->	columnIndex
	///////////////////////////////////////////////////////////////
	
	//iterate through m_worldGrid and draw tiles within index bound
	SDL_Rect drawRect;
	drawRect.x = 0;
	drawRect.y = 0;
	uint32_t currentRow = topNonCulledRowIndex;
	for ( uint32_t index = topNonCulledRowIndex * Globals::WORLD_X_SIZE + leftNonCulledColIndex; //index starts at top-left element in non-culled part of grid
		  index < bottomNonCulledRowIndex * Globals::WORLD_X_SIZE + (rightNonCulledColIndex+1);  //index terminates at bottom-right element in non-culled part of grid
		)
	{
		//set dimensions for this tile
		drawRect.w = pixelsPerTileDim;
		drawRect.h = pixelsPerTileDim;
		if (index / Globals::WORLD_X_SIZE == topNonCulledRowIndex)
			drawRect.h = static_cast<int>(topRowHeightWithinBound);
		if (index / Globals::WORLD_X_SIZE == bottomNonCulledRowIndex) 
			drawRect.h = static_cast<int>(bottomRowHeightWithinBound);
		if (index % Globals::WORLD_X_SIZE == leftNonCulledColIndex)
			drawRect.w = static_cast<int>(leftColWidthWithinBound);
		if (index % Globals::WORLD_X_SIZE == rightNonCulledColIndex)
			drawRect.w = static_cast<int>(leftColWidthWithinBound);

		// black for 0, white for 1
		flPoint const& pointAtIndex = m_worldGrid[index];
		ValRGBA drawColour;
		drawColour.a = 255u;
		if (pointAtIndex.m_energy == 1)
		{
			drawColour.r = 255;
			drawColour.g = 255;
			drawColour.b = 255;
		}
		else if (pointAtIndex.m_energy == 0)
		{
			drawColour.r = 0;
			drawColour.g = 0;
			drawColour.b = 0;
			
		}
		SDL_SetRenderDrawColor(_gRenderer, drawColour.r, drawColour.g, drawColour.b, drawColour.a);
		SDL_RenderFillRect(_gRenderer, &drawRect);

		//Need to fit tiles to some bounds within the screen space - fullscreen feels best as default
		//Globals::WINDOW_WIDTH and WINDOOW_HEIGHT are passed into SDL_CreateWindow(...) so are literally the pixel
		//dimensions of the Window that gets displayed on screen (i.e. same dimensions will take up a larger screen space
		//on smaller monitors). Remember, Window's pixel array starts at index (0, 0) and goes to 
		//(WINDOW_WIDTH-1, WINDOW_HEIGHT-1)
			

		drawRect.x += drawRect.w;
		index++;
		if (index % Globals::WORLD_X_SIZE == rightNonCulledColIndex)
		{
			//at end of row, move down to first element of row below
			drawRect.x = 0;
			drawRect.y += drawRect.h;

			currentRow++;
			index = currentRow * Globals::WORLD_X_SIZE + leftNonCulledColIndex;
		}
	}

	/*
	SDL_SetRenderDrawColor(_gRenderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(_gRenderer, m_linePos1.x, m_linePos1.y+m_border, m_linePos2.x , m_linePos2.y-m_border);
	*/
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void WorldGrid::DefineHeldInput()
{
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		std::cout << "Left Pressed Game Mode" << std::endl;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		std::cout << "Right Pressed Game Mode" << std::endl;
	}
}

void WorldGrid::KeyPressedInput(SDL_Scancode const& _key)
{
	if (_key == SDL_SCANCODE_UP)
	{
		m_border -= 1;
	}
	if (_key == SDL_SCANCODE_DOWN)
	{
		m_border += 1;
	}
	if (_key == SDL_SCANCODE_R)
	{
		m_border = 0;
	}
}
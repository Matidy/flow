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
		p.m_energy = (i + ((i/Globals::WORLD_X_SIZE)%2)) % 2; //each tile checkerboard

		/*p.m_energy = rand() % 2;*/ //random
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
	//1. Using m_worldGrid, find every flPoint within m_cullingViewport's bounds.
	//2. For each sqaure, create a draw square
	//		i. for sqaures at edge of viewport, set edge points to be equal to culling bounds.

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

	flVec2<float> const offsetsFromCenterToEdge = GetCenterToEdgeOffsets();

	//get row index for FIRST ROW INSIDE viewport and how much height off that row is not culled
	uint32_t topNonCulledRowIndex = 0; //start from top most row
	float topRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportTopY = m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension;
		float curRowBottomY = offsetsFromCenterToEdge.y - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with bottom edge of top row

		//move down through rows until we find one at least partly within cull boundary
		while (curRowBottomY >= viewportTopY)
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
			topRowHeightWithinBound -= abs(viewportTopY - curRowBottomY);
		}
	}
	//get row index for LAST ROW INSIDE viewport, and how much height off that row is not culled
	int32_t bottomNonCulledRowIndex = Globals::WORLD_Y_SIZE-1; //start from bottom most row
	float bottomRowHeightWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of row is within bounds to start
	{
		float viewportBottomY = m_cullingViewport.m_pos.y - m_cullingViewport.m_yExtension;
		float curRowTopY = -offsetsFromCenterToEdge.y + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with top edge of bottom row

		//move up through rows until we find one at least partly within cull boundary
		while (curRowTopY <= viewportBottomY)
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
			bottomRowHeightWithinBound -= abs(viewportBottomY - curRowTopY);
		}
	}
	//get column index for LEFT-MOST COLUMN INSIDE viewport, and how much width off that column is not culled
	uint32_t leftNonCulledColIndex = 0; //start from left-most column
	float leftColWidthWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of column is within bounds to start
	{
		float viewportLeftX = m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension;
		float curColRightX = -offsetsFromCenterToEdge.x + static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with right edge of left-most column

		//move right through columns until we find one at least partly within cull boundary
		while (curColRightX <= viewportLeftX)
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
			leftColWidthWithinBound -= abs(viewportLeftX - curColRightX);
		}
	}
	//get column index for RIGHT-MOST COLUMN INSIDE viewport, and how much width off that column is not culled
	int32_t rightNonCulledColIndex = Globals::WORLD_X_SIZE-1; //start from right-most column
	float rightColWidthWithinBound = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //assume all of column is within bounds to start
	{
		float viewportRightX = m_cullingViewport.m_pos.x + m_cullingViewport.m_xExtension;
		float curColLeftX = offsetsFromCenterToEdge.x - static_cast<float>(Globals::TILE_DRAW_DIMENSIONS); //start with left edge of right-most column

		//move left through columns until we find one at least partly within cull boundary
		while (curColLeftX >= viewportRightX)
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
			rightColWidthWithinBound -= abs(viewportRightX - curColLeftX);
		}
	}

	//if viewport totally contains world, get border between edge of world and viewport bounds
	//top
	float distTopViewportToWorld = 0.f;
	if (offsetsFromCenterToEdge.y < m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension)
		distTopViewportToWorld = (m_cullingViewport.m_pos.y + m_cullingViewport.m_yExtension) - offsetsFromCenterToEdge.y;
	//left
	float distLeftViewportToWorld = 0.f;
	if (-offsetsFromCenterToEdge.x > m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension)
		distLeftViewportToWorld = abs((m_cullingViewport.m_pos.x - m_cullingViewport.m_xExtension) + offsetsFromCenterToEdge.x); 

	//@cleanup - don't need these two as start drawing world from top left
	/*
	//bottom
	float distBottomWorldToViewport = 0.f;
	if (-offsetsFromCenterToEdge.y > m_cullingViewport.m_pos.y - m_cullingViewport.m_yExtension)
		distBottomWorldToViewport = (m_cullingViewport.m_pos.y - m_cullingViewport.m_yExtension) + offsetsFromCenterToEdge.y; //previous line allows guarantee that viewport bottom bound is negative
	*/
	/*
	//right
	float distRightWorldToViewport = 0.f;
	if (offsetsFromCenterToEdge.x < m_cullingViewport.m_pos.x + m_cullingViewport.m_xExtension)
		distRightWorldToViewport = (m_cullingViewport.m_pos.x + m_cullingViewport.m_xExtension) - offsetsFromCenterToEdge.x;
	*/

	//@consider - probably shouldn't use float values when doing culling operations as will then have to convert from float to int values when scaling culled worldGrid to
	//			  our window/screen's pixel array. Using floats obfuscates whole process and makes rounding errors/skipped pixels possible.
	/*
	float nonCulledWorldWidth = static_cast<float>((rightNonCulledColIndex - leftNonCulledColIndex - 1)*Globals::TILE_DRAW_DIMENSIONS) + leftColWidthWithinBound + rightColWidthWithinBound;
	//given tiles are square, calc how many pixels high/wide we should draw each tile to fill the screen
	float pixelsPerTileDim = static_cast<float>(Globals::WINDOW_WIDTH)
						   / nonCulledWorldWidth; 
	*/

	//@next - is this the correct way to scale from viewport/world dimensions to screen dimensions?
	

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
	
	
	if (topNonCulledRowIndex < Globals::WORLD_Y_SIZE
		&& bottomNonCulledRowIndex >= 0
		&& leftNonCulledColIndex < Globals::WORLD_X_SIZE
		&& rightNonCulledColIndex >= 0) //check viewport actually contains some part of the world
	{
		float viewportToWindowScalar = Globals::WINDOW_WIDTH / (m_cullingViewport.m_xExtension * 2);
		float pixelsPerTileDim = Globals::TILE_DRAW_DIMENSIONS * viewportToWindowScalar;

		SDL_Rect drawRect;
		float pixelsLeftWorldBorder = distLeftViewportToWorld * viewportToWindowScalar;
		float pixelsTopWorldBorder = distTopViewportToWorld * viewportToWindowScalar;
		drawRect.x = 0u + FloatToPixelsX(pixelsLeftWorldBorder, true);
		drawRect.y = 0u + FloatToPixelsY(pixelsTopWorldBorder, true);

		drawRect.h = FloatToPixelsY(pixelsPerTileDim); //calc height for first row

		uint32_t currentRow = topNonCulledRowIndex;

		//iterate through m_worldGrid and draw tiles within index bound
		for (uint32_t index = topNonCulledRowIndex * Globals::WORLD_X_SIZE + leftNonCulledColIndex; //index starts at top-left element in non-culled part of grid
			index < bottomNonCulledRowIndex * Globals::WORLD_X_SIZE + (rightNonCulledColIndex + 1);  //index terminates at bottom-right element in non-culled part of grid
			)
		{
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
				
				//set height for next row
				float tilePixelHeight = pixelsPerTileDim;
				if (index / Globals::WORLD_X_SIZE == topNonCulledRowIndex)
					tilePixelHeight = topRowHeightWithinBound * viewportToWindowScalar;
				if (index / Globals::WORLD_X_SIZE == bottomNonCulledRowIndex)	
					tilePixelHeight = bottomRowHeightWithinBound * viewportToWindowScalar;
				drawRect.h = FloatToPixelsY(tilePixelHeight);

				currentRow++;
				index = currentRow * Globals::WORLD_X_SIZE + leftNonCulledColIndex;
			}
		}
	}
}

/////////////////////				   ///////////////////////////////////////////////////////
//////////////////   from InputDelegate   ///////////////////////////////////////////////////
/////////////////////				   //////////////////////////
void WorldGrid::DefineHeldInput()
{
#ifdef _DEBUG
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		std::cout << "Left Pressed Game Mode" << std::endl;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		std::cout << "Right Pressed Game Mode" << std::endl;
	}
#endif

	//viewport controls - pan
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_W) || m_inputCoreIF.IsPressed(SDL_SCANCODE_UP))
	{
		m_cullingViewport.m_pos.y += m_cullingViewport.m_yExtension * 0.05f;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_S) || m_inputCoreIF.IsPressed(SDL_SCANCODE_DOWN))
	{
		m_cullingViewport.m_pos.y -= m_cullingViewport.m_yExtension * 0.05f;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_A) || m_inputCoreIF.IsPressed(SDL_SCANCODE_LEFT))
	{
		m_cullingViewport.m_pos.x -= m_cullingViewport.m_xExtension * 0.05f;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_D) || m_inputCoreIF.IsPressed(SDL_SCANCODE_RIGHT))
	{
		m_cullingViewport.m_pos.x += m_cullingViewport.m_xExtension * 0.05f;
	}
	//zoom in/out
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_Q))
	{
		m_cullingViewport.m_xExtension = m_cullingViewport.m_xExtension * 1.01f;
		m_cullingViewport.m_yExtension = m_cullingViewport.m_yExtension * 1.01f;
	}
	if (m_inputCoreIF.IsPressed(SDL_SCANCODE_E))
	{
		m_cullingViewport.m_xExtension = m_cullingViewport.m_xExtension * 0.99f;
		m_cullingViewport.m_yExtension = m_cullingViewport.m_yExtension * 0.99f;
	}
}

void WorldGrid::KeyPressedInput(SDL_Scancode const& _key)
{

}
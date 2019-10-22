#include <vector>

#include <SDL_rect.h>

#include "Globals.h"
#include "Data/flEnergy.h"
#include "Data/flSpace.h"
#include "Data/flVec2.h"
#include "Libs/PhysicsLib.h"
#include "RenderDelegate.h"
#include "InputDelegate.h"

struct RenderCoreIF;
struct InputCoreIF;

class WorldGrid : 
	public RenderDelegate,
	public InputDelegate
{
public:
	//from RenderDelegate
	virtual void DelegateDraw(SDL_Renderer * const _gRenderer) const override final;

	//from InputDelegate
	virtual void DefineHeldInput(uint32_t _timeStep) override final;
	virtual void KeyPressedInput(SDL_Scancode const& _key) override final;
	virtual void DefineChordInput(uint32_t _timeStep) override final;
	virtual void MouseDownInput(eMouseButtonType const _buttonType, flVec2<int> _mousePos) override final;
	virtual void MouseUpInput(eMouseButtonType const _buttonType, flVec2<int> _mousePos) override final;

	WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF);
	~WorldGrid();

	bool GenerateWorld();
	bool UpdateStep(uint32_t const _timeStep);

private:
	void UpdateClaimRectBounds(flVec2<int> _mousePos);
	void ClaimTiles();

	flVec2<flEnergy::MoveVectorType> const GetCenterToEdgeOffsets() const;

	flVec2<int32_t>	Pos1DToPos2DInt(uint32_t _index);
	flVec2<flEnergy::MoveVectorType> Pos1DToPos2D(uint32_t _index);
	uint32_t Pos2DToPos1D(flVec2<int32_t> _pos2D);
	flSpace& GetSpaceAtIndex(uint32_t _index);
	flEnergy& GetEnergyAtIndex(uint32_t _index);
	uint32_t GetIndexUp(uint32_t _currentPointIndex) const;
	uint32_t GetIndexDown(uint32_t _currentPointIndex) const;
	uint32_t GetIndexLeft(uint32_t _currentPointIndex) const;
	uint32_t GetIndexRight(uint32_t _currentPointIndex) const;

	flVec2<float> PixelPosInTileWorld(uint32_t _pixelX, uint32_t _pixelY) const;
	flVec2<float> TilePosToPixelPos(int32_t _col, int32_t _row) const;
	float GetPixelToTileScalar() const { return (m_cullingViewport.m_xExtension*2)/Globals::WINDOW_WIDTH; }
	float GetTileToPixelScalar() const { return Globals::WINDOW_WIDTH/(m_cullingViewport.m_xExtension*2); }

	/**** world array data ****
	* initially just a contiguous array, accessing different arrays using width/height world constants
	* future challange is how to store Point data when looking to expand the boundaries/size of the world,
	* e.g. - increase resolution of a space within the world bounds
	*     - add more world to left of existing world: if new world data is B and old is A then expansion
	*       pattern of world data is from [Ar1-Ar2-Ar3...Arn] to [Br1-Ar1-Br2-Ar2-Br3-Ar3...Brn-Arn].
	*/
	std::vector<flEnergy> m_worldEnergy; 
	std::vector<flSpace> m_worldGrid; //using vector for debugger readout
	flEnergy m_nullEnergy;
	flSpace m_nullSpace;
	uint32_t const m_energyToSpaceRatio = 128;
	uint32_t m_cumulativeFrameTime = 0;

	//dimensions of mouse cursor quad in pixels
	flVec2<float> m_mouseCursorDim;
	SDL_Point m_cursorBox[5];
	bool m_cursorInWorld = false;

	enum eClaimState
	{
		NoOp1		=	0,
		Unclaim		=	1 << 0,
		Claim		=	1 << 1,
		NoOp2		=   3
	};
	uint8_t m_claimBitMap = 0u;

	struct ClaimRect
	{
		 int32_t m_leftCol = -1;
		 int32_t m_topRow = -1;
		 int32_t m_rightCol = -1;
		 int32_t m_bottomRow = -1;

		 eClaimState m_associatedClaimState = NoOp1; //what change to tile state this data is associated with
	};
	ClaimRect m_claimRect;

	struct CullingViewport
	{
		static constexpr float m_xExtensionDefault = static_cast<float>(Globals::TILE_DRAW_DIMENSIONS*Globals::WORLD_X_SIZE) + 16.f;
		static constexpr float m_yExtensionDefault = m_xExtensionDefault * Globals::WINDOW_ASPECT_RATIO;
		//as long as m_xExtension >= 16, is a power of 2, and we're using a 16:9 aspect ratio, then m_yExtension will be a whole number

		flVec2<float> m_pos;
		float m_xExtension = m_xExtensionDefault; //deviation from m_pos left and right to define extent of viewport
		float m_yExtension = m_yExtensionDefault; //deviation from m_pos up and down to define extent of viewport

#ifdef _DEBUG
		float m_debugPanIncrement = 0.5f;
#endif
		CullingViewport(flVec2<float> _pos)
			: m_pos(_pos)
		{}

		void Reset() { m_pos.x = 0.f; m_pos.y = 0.f; m_xExtension = m_xExtensionDefault; m_yExtension = m_yExtensionDefault; }

	};
	CullingViewport m_cullingViewport;

	float m_panSpeed = 2.f;
	float m_zoomSpeed = 1.f;
};

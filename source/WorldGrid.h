#include "Globals.h"
#include "Data/flPoint.h"
#include "Data/flVec2.h"
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

	WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF);
	~WorldGrid();

	bool GenerateWorld();
	bool UpdateStep(uint32_t const _timeStep);

private:
	flVec2<float> const GetCenterToEdgeOffsets() const;

	flPoint& GetPointUp(uint32_t _currentPointIndex);
	flPoint& GetPointDown(uint32_t _currentPointIndex);
	flPoint& GetPointLeft(uint32_t _currentPointIndex);
	flPoint& GetPointRight(uint32_t _currentPointIndex);

	//initially just a contiguous array, accessing different arrays using width/height world constants
	//future challange is how to store Point data when looking to expand the boundaries/size of the world,
	//e.g. - increase resolution of a space within the world bounds
	//     - add more world to left of existing world: if new world data is B and old is A then expansion
	//       pattern of world data is from [Ar1-Ar2-Ar3...Arn] to [Br1-Ar1-Br2-Ar2-Br3-Ar3...Brn-Arn].
	flPoint* m_worldGrid;
	flPoint m_nullPoint;

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

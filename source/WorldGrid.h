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
	virtual void DefineHeldInput() override final;
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
		flVec2<float> m_pos;
		float m_xExtension = static_cast<float>(Globals::WINDOW_WIDTH)/2; //deviation from m_pos left and right to define extent of viewport
		float m_yExtension = static_cast<float>(Globals::WINDOW_HEIGHT)/2; //deviation from m_pos up and down to define extent of viewport

		CullingViewport(flVec2<float> _pos)
			: m_pos(_pos)
		{}
	};
	CullingViewport m_cullingViewport;

	int m_border = 0;
	flVec2<int> m_linePos1 = flVec2<int>(Globals::WINDOW_WIDTH / 2, 0);
	flVec2<int> m_linePos2 = flVec2<int>(Globals::WINDOW_WIDTH / 2, Globals::WINDOW_HEIGHT-1);
};

#include "Globals.h"
#include "Data/flPoint.h"
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
	void virtual DelegateDraw(SDL_Renderer * const _gRenderer) const override final;

	//from InputDelegate
	void virtual DefineHeldInput() override final;

	WorldGrid(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF);
	~WorldGrid();

	bool GenerateWorld();
	bool UpdateStep(uint32_t const _timeStep);

private:
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
		flVec2 m_pos;
		uint32_t m_width = Globals::WINDOW_WIDTH;
		uint32_t m_height = Globals::WINDOW_HEIGHT;

		CullingViewport(flVec2 _pos)
			: m_pos(_pos)
		{}
	};
	CullingViewport m_cullingViewport;
};

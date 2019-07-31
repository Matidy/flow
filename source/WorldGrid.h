#include "Data/flPoint.h"
#include "InputDelegate.h"

#define WORLD_X_SIZE (2^12)
#define WORLD_Y_SIZE (2^12)
#define TOTAL_WORLD_SIZE WORLD_X_SIZE*WORLD_Y_SIZE

struct InputCoreIF;

class WorldGrid : public InputDelegate
{
public:
	//from InputDelegate
	void virtual DefineHeldInput() override final;

	WorldGrid(InputCoreIF& _inputCore);

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
	flPoint m_worldGrid[TOTAL_WORLD_SIZE];

	flPoint m_nullPoint;
};

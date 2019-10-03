#ifndef FL_ENERGY
#define FL_ENERGY

#include "flVec2.h"
#include "../Libs/PhysicsLib.h"
#include "../Globals.h"


struct flEnergy
{
public:
	//@optimise - storing very large array of these, meaning every byte extra here gets multiplied by Globals::TOTAL_WORLD_SIZE

	/* going with axis aligned x,y square tessalated grid for simplicity sake - conforms to existing knowledge/
	*  understanding of 2D vector maths/trigonometry.
	*
	*  array of points, use relative jumps to start with, i.e. n+rowLength = down.
	*  other posibility is to have a point class that stores points to its up, left, right, down adjacent points
	*  but this feels overkill for now */

	/* currently only 0+ values of energy. future changes could be: 
	*	- to scale attraction/repulsion force based on amount of energy
	*	- to handle negative values of energy as flipping repulsive force to attractive */
	int16_t m_energy; //up to 16,384

	/* associated flSpace value in WorldGrid::m_worldGrid. Informs where this flEnergy is located in our world.
	*  Currently need this to tell associated flSpace point what this flEnergy's index is in energy array after we've sorted it.
	*  Design of world movement/access means we should only really be accessing energy via its associated flSpace element */
	uint32_t m_indexInWorld;

	typedef float PointVectorType;
	PhysicsLib::Vector2D<PointVectorType> m_movementVector; //storing as float as movement per frame is sub division of movement per second
	flVec2<PointVectorType> m_currentMovementStore; //stores how much of current movement iteration we've completed
	uint32_t m_accumulatedMoveTime;

	//whether or not this point reflects all energy that interacts with it, i.e. if it is 'moveable' or not
	bool m_moveable;

	//possibly only debug val, used to direct point propagation movement
	enum Direction : uint8_t
	{
		UP,
		DOWN,
		LEFT,
		RIGHT,
		ALL
	} m_direction = ALL;
	
	flEnergy
	(
		int16_t _energy,
		PhysicsLib::Vector2D<PointVectorType> _movVec,
		bool _moveable
	)
		: m_energy(_energy),
		m_indexInWorld(Globals::NULL_INDEX),
		m_movementVector(_movVec),
		m_currentMovementStore(),
		m_accumulatedMoveTime(0u),
		m_moveable(_moveable)
	{}

	flEnergy()
		: m_energy(0),
		m_indexInWorld(Globals::NULL_INDEX),
		m_movementVector(),
		m_currentMovementStore(),
		m_accumulatedMoveTime(0u),
		m_moveable(true)
	{}

	void Nullify()
	{
		m_energy = 0;
		m_movementVector = PhysicsLib::Vector2D<PointVectorType>();
	}

private:

	// -----OLD----- //
	/* start with equalateral triangle tessalated grid
	*  figuring out first guess way to implement points referencing points around them.
	*		iterate through points, each point stores refs to neighbours, each point then propagates a force along those neighbours, iterating
	*		in each neighbour's direction until the exerted force becomes 0. Possible optimisation is that if a point has not been processed yet
	*		the force is cached on that node and then propagated with that node's own force when it is processed. Limited force range seems a lot
	*		better than O(n^2) as n grows. May be able to group points for large force over distance. Other approach is to use field approach
	*		potentially looking at Newtonian gravity sims for inspiration (but then you still need to calculate the force exerted from gravity
	*		function so will it actually save any computation).
	* flEnergy m_ */
};
#endif //ifndef flEnergy
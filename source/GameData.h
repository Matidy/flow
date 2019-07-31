#pragma once
#include "InputDelegate.h"
#include "WorldGrid.h"
#ifdef _DEBUG
#include "Debug.h"
#endif

struct RenderCoreIF;
struct InputCoreIF;
class WorldGrid;
#ifdef _DEBUG
class Debug;
#endif 

class GameData : public InputDelegate
{
	friend class RenderCore;

public:
	//from InputDelegate
	virtual void DefineChordInput() override final;

	GameData(RenderCoreIF& _renderCore, InputCoreIF& _inputCore);

	void UpdateStep(uint32_t const _timeStep);
	
private:
	WorldGrid m_worldGrid;
#ifdef _DEBUG
	Debug m_debug;
#endif

	RenderCoreIF& m_renderCoreIF;
};
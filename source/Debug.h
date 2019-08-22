#include <vector>
#include <SDL_scancode.h>

#include "Data/flPoint.h"
#include "RenderDelegate.h"
#include "InputDelegate.h"

struct RenderCoreIF;
struct InputCoreIF;

class Debug : 
	public RenderDelegate,
	public InputDelegate
{
public:
	//from RenderDelegate
	virtual void DelegateDraw(SDL_Renderer * const _gRenderer) const override final;

	//from InputDelegate
	virtual void DefineHeldInput(uint32_t _timeStep) override final;
	virtual void KeyPressedInput(SDL_Scancode const& _key) override final;
	virtual void KeyReleasedInput(SDL_Scancode const& _key) override final;

	static constexpr uint32_t m_debugWorldDim = 17u;
	static constexpr uint32_t m_debugWorldSize = m_debugWorldDim * m_debugWorldDim;

	Debug(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF);
	~Debug();
	void UpdateStep(uint32_t const _timeStep);

	bool const& ToggleDebug();
	bool const DebugEnabled() const;

private:
	enum PropagationMode
	{
		SimpleDirectional,
		SemiCircleDirectional,

		CountPropagationMode
	};
	uint32_t m_propagationMode;

	enum class DrawMode
	{
		Propagation,
		Tessalation
	};
	DrawMode m_drawMode;

	static constexpr uint32_t m_nullIndex = ~0;

	flPoint* m_debugWorld;
	bool m_debugEnabled;

	SDL_Point* m_tessalationPoints;

	float m_propagationRate;
	std::vector<uint32_t> m_pointsToPropagate;

	void CyclePropModeLeft();
	void CyclePropModeRight();
	void PropagateAdjacent();
	void UnpropagateAdjacent();
	void ResetWorldGrid();

	uint32_t GetIndexUp(uint32_t _currentPointIndex);
	uint32_t GetIndexDown(uint32_t _currentPointIndex);
	uint32_t GetIndexLeft(uint32_t _currentPointIndex);
	uint32_t GetIndexRight(uint32_t _currentPointIndex);
};

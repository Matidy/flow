#include "flPoint.h"

class Debug
{
public:
	Debug();

	void PropagateAdjacent();
	void ToggleDebug();
	bool DebugEnabled();

private:
	flPoint m_debugWorld[17*17];
	bool m_debugEnabled;
};

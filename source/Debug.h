#include "flPoint.h"

class Debug
{
public:
	Debug();

	void PropagateAdjacent();

	void ToggleDebug();
	bool const DebugEnabled() const;

	void ToggleTessalationDisplay();
	bool const DisplayTessalation() const;

private:
	flPoint m_debugWorld[17*17];
	bool m_debugEnabled;
	//used to tell Window whether it should be displaying tessalation screen or not
	bool m_displayTessalation;
};

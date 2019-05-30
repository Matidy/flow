#include <stdio.h>
#include "Debug.h"

Debug::Debug()
	: m_debugEnabled(false),
	m_displayTessalation(false)
{
}

void Debug::ToggleDebug
(
)
{
	m_debugEnabled = !m_debugEnabled;
	if (m_debugEnabled)
	{
		printf("Debug Enabled\n");
	}
	else
	{
		printf("Debug Disabled\n");
	}
}

bool const Debug::DebugEnabled
(
) const
{
	return m_debugEnabled;
}

void Debug::ToggleTessalationDisplay
(
)
{
	m_displayTessalation = !m_displayTessalation;
}

bool const Debug::DisplayTessalation
(
) const
{
	return m_displayTessalation;
}

void Debug::PropagateAdjacent
(
)
{

}
#include <stdio.h>
#include "Debug.h"

Debug::Debug()
	: m_debugEnabled(false)
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

bool Debug::DebugEnabled
(
)
{
	return m_debugEnabled;
}

void Debug::PropagateAdjacent
(
)
{

}
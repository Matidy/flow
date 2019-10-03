#pragma once

#include "../Globals.h"

struct flSpace
{
	/* index of flEnergy this flSpace part contains, in flEnergy array
	*		fixed and sorted energy size array so index okay way to go for now (maybe consider ID/hashmap in future) */
	uint32_t m_energyIndex = Globals::NULL_INDEX; 
};
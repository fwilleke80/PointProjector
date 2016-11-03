#include "c4d.h"
#include "wsPointProjector.h"
#include "main.h"


Bool PluginStart(void)
{
	if (!RegisterProjectorObject()) return false;

	return true;
}

void PluginEnd(void)
{
}

Bool PluginMessage(Int32 id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			return true;
	}

	return false;
}

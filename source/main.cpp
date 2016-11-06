#include "c4d.h"
#include "c4d_symbols.h"
#include "wsPointProjector.h"
#include "main.h"

#define PLUGIN_NAME "SplineProjector 1.1"

Bool PluginStart(void)
{
	GePrint(GeLoadString(IDS_LOADING, PLUGIN_NAME));
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
